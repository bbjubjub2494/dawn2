use alloy::network::{Ethereum, EthereumWallet, NetworkWallet};
use alloy::primitives::*;
use alloy::providers::{Provider, ProviderBuilder, WsConnect};
use alloy::signers::local::{coins_bip39::English, MnemonicBuilder};
use alloy::sol;
use alloy::sol_types::SolEvent;
use alloy::transports::Transport;
use alloy::consensus::{dawn, TypedTransaction};
use alloy::rpc::types::TransactionRequest;
use eyre::Result;

use futures_util::StreamExt;

use dawn_enclave_protocol::{MasterPublicKey, SealedMasterPrivateKey};

sol!(
    #[allow(missing_docs)]
    #[sol(rpc)]
    SimpleAuctions,
    "../contracts/out/SimpleAuctions.sol/SimpleAuctions.json"
);

sol!(
    #[allow(missing_docs)]
    #[sol(rpc)]
    Collection,
    "../contracts/out/Collection.sol/Collection.json"
);

sol!(
    #[allow(missing_docs)]
    #[sol(rpc)]
    WETH,
    "../contracts/out/WETH.sol/WETH.json"
);

#[tokio::main]
async fn main() -> Result<()> {
    let scenario = Scenario::new().await?;

    let bidders = (1..20)
        .map(|i| tokio::spawn(async move { scenario.bidder_script(i).await }))
        .collect::<Vec<_>>();
    scenario.operator_script().await?;

    for bidder in bidders {
        bidder.await??;
    }

    Ok(())
}

#[derive(Debug, Clone, Copy)]
struct Scenario {
    auctions_address: Address,
    collection_address: Address,
    weth_address: Address,
    mpk: MasterPublicKey,
}

impl Scenario {
    async fn new() -> Result<Self> {
        let (mpk, _) = load_master_key();
        let (deployer_wallet, deployer_address) = derive_key(0)?;
        let provider = &ProviderBuilder::new()
            .with_recommended_fillers()
            .wallet(deployer_wallet)
            .on_ws(WsConnect::new("ws://localhost:8546"))
            .await?;

        // Deploy the contract.
        let block_delay = 2;
        let auctions = SimpleAuctions::deploy(provider, block_delay).await?;
        let collection = Collection::deploy(provider).await?;
        let weth = WETH::deploy(provider).await?;

        Ok(Scenario {
            auctions_address: *auctions.address(),
            collection_address: *collection.address(),
            weth_address: *weth.address(),
            mpk,
        })
    }

    async fn operator_script(&self) -> Result<()> {
        let amount = U256::from(1);
        let (operator_wallet, deployer_address) = derive_key(0)?;
        let provider = &ProviderBuilder::new()
            .with_recommended_fillers()
            .wallet(operator_wallet)
            .on_ws(WsConnect::new("ws://localhost:8546"))
            .await?;
        let (auctions, collection, weth) = self.bindings(provider);

        weth.deposit().value(amount).send().await?.watch().await?;
        weth.approve(*auctions.address(), amount)
            .send()
            .await?
            .watch()
            .await?;

        let token_id = U256::from(1);
        collection
            .approve(*auctions.address(), token_id)
            .send()
            .await?
            .watch()
            .await?;

        let r = auctions
            .startAuction(
                *collection.address(),
                token_id,
                *weth.address(),
                deployer_address,
            )
            .send()
            .await?
            .get_receipt()
            .await?;
        let Some(ev) = r.inner.logs().iter().find_map({
            |l| {
                if l.topic0() == Some(&SimpleAuctions::AuctionStarted::SIGNATURE_HASH) {
                    Some(SimpleAuctions::AuctionStarted::decode_raw_log(
                        l.topics(),
                        &l.data().data,
                        false,
                    ))
                } else {
                    None
                }
            }
        }) else {
            return Err(eyre::eyre!("startAuction() did not emit event"));
        };

        let SimpleAuctions::AuctionStarted {
            auctionId,
            revealDeadline,
            ..
        } = ev?;
        wait_for_block(provider, revealDeadline).await?;
        auctions
            .settle(auctionId)
            .send()
            .await?
            .get_receipt()
            .await?;

        Ok(())
    }

    async fn bidder_script(&self, index: u32) -> Result<()> {
        let (bidder_wallet, bidder_address) = derive_key(index)?;
        let provider = &ProviderBuilder::new()
            .with_recommended_fillers()
            .wallet(bidder_wallet.clone())
            .on_ws(WsConnect::new("ws://localhost:8546"))
            .await?;
        let (auctions, collection, weth) = self.bindings(provider);

        let amount = U256::from(2);
        weth.deposit().value(amount).send().await?.watch().await?;
        weth.approve(*auctions.address(), amount)
            .send()
            .await?
            .watch()
            .await?;

        let (auction_id, opening) = {
            let auction_filter = auctions.AuctionStarted_filter().watch().await?;
            let mut stream = auction_filter.into_stream();
            let Some(event) = stream.next().await else {
                return Err(eyre::eyre!("No auction started event"));
            };
            let info = event?.0;
            (info.auctionId, info.opening)
        };

        wait_for_block(provider, opening).await?;
        let tx = TransactionRequest{
            chain_id: Some(1337),
            from: Some(bidder_address),
            nonce: Some(2),
            gas_price: Some(1_000_000_000),
            gas: Some(1_000_000),
            ..auctions.bid(auction_id, amount).into_transaction_request()}.build_typed_tx().unwrap();
        
        let tx = dawn::encrypt(&self.mpk, tx.legacy().unwrap(), &bidder_address);
        let tx = TypedTransaction::DawnEncrypted(tx);
        let tx = <EthereumWallet as NetworkWallet<Ethereum>>::sign_transaction(&bidder_wallet, tx).await?;
        let r = provider.send_tx_envelope(tx).await?.get_receipt().await?;
        dbg!(r);

        Ok(())
    }

    fn bindings<'a, T: Transport + Clone, P: Provider<T, Ethereum>>(
        &self,
        provider: &'a P,
    ) -> (
        SimpleAuctions::SimpleAuctionsInstance<T, &'a P>,
        Collection::CollectionInstance<T, &'a P>,
        WETH::WETHInstance<T, &'a P>,
    ) {
        (
            SimpleAuctions::new(self.auctions_address, provider),
            Collection::new(self.collection_address, provider),
            WETH::new(self.weth_address, provider),
        )
    }
}

fn derive_key(index: u32) -> Result<(EthereumWallet, Address)> {
    let mnemonic_phrase = "test test test test test test test test test test test junk".to_string();
    let signer = MnemonicBuilder::<English>::default()
        .phrase(mnemonic_phrase)
        .index(index)?
        .build()?;
    let address = signer.address();
    Ok((EthereumWallet::from(signer), address))
}

async fn wait_for_block<T: Transport + Clone>(
    provider: &impl Provider<T, Ethereum>,
    block_number: u64,
) -> Result<()> {
    let subscription = provider.subscribe_blocks().await?;
    let mut stream = subscription.into_stream();

    while let Some(block) = stream.next().await {
        if block.header.number > Some(block_number) {
            break;
        }
    }
    Ok(())
}

fn load_master_key() -> (MasterPublicKey, SealedMasterPrivateKey) {
    serde_json::from_str(&std::env::var("DAWN_MASTER_KEY").expect("DAWN_MASTER_KEY not set"))
        .unwrap()
}
