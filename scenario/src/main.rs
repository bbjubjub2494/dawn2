use alloy::network::EthereumWallet;
use alloy::primitives::*;
use alloy::providers::{Provider, ProviderBuilder, WsConnect};
use alloy::signers::local::{coins_bip39::English, MnemonicBuilder};
use alloy::sol;
use eyre::Result;

use futures_util::StreamExt;

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

fn derive_key(index: u32) -> Result<(EthereumWallet, Address)> {
    let phrase = "test test test test test test test test test test test junk"; // reth default mnemonic
    let signer = MnemonicBuilder::<English>::default()
        .phrase(phrase)
        .index(index)?
        .build()?;
    let address = signer.address();
    Ok((EthereumWallet::from(signer), address))
}

#[tokio::main]
async fn main() -> Result<()> {
    let rpc_url = std::env::var("ETH_RPC_URL")?.parse()?;
    let (deployer_wallet, deployer_address) = derive_key(0)?;
    let provider = &ProviderBuilder::new()
        .with_recommended_fillers()
        .wallet(deployer_wallet)
        .on_http(rpc_url);

    // Deploy the contract.
    let block_delay = 2;
    let auctions = SimpleAuctions::deploy(provider, block_delay).await?;
    dbg!(auctions.address());

    let collection = Collection::deploy(provider).await?;
    dbg!(collection.address());

    collection
        .approve(*auctions.address(), U256::from(1))
        .send()
        .await?
        .watch()
        .await?;
    let r = auctions
        .startAuction(*collection.address(), U256::from(1), deployer_address)
        .value(U256::from(1))
        .send()
        .await?
        .get_receipt()
        .await?;
    dbg!(r);

    tokio::spawn(bidder_script(*auctions.address(), *collection.address(), 1)).await??;

    Ok(())
}

async fn bidder_script(
    auctions_address: Address,
    collection_address: Address,
    index: u32,
) -> Result<()> {
    let (bidder_wallet, bidder_address) = derive_key(index)?;
    let provider = &ProviderBuilder::new()
        .with_recommended_fillers()
        .wallet(bidder_wallet)
        .on_ws(WsConnect::new("ws://localhost:8546"))
        .await?;

    let auctions = SimpleAuctions::new(auctions_address, provider);
    let collection = Collection::new(auctions_address, provider);

    let (auction_id, opening) = {
        let auction_filter = auctions.AuctionStarted_filter().watch().await?;
        let mut stream = auction_filter.into_stream();
        let Some(event) = stream.next().await else {
            return Err(eyre::eyre!("No auction started event"));
        };
        let info = event?.0;
        (info.auctionId, info.opening)
    };

    {
        let subscription = provider.subscribe_blocks().await?;
        let mut stream = subscription.into_stream();

        while let Some(block) = stream.next().await {
            if block.header.number > Some(opening) {
                break;
            }
        }
    }

    let r = auctions
        .bid(auction_id)
        .value(U256::from(2))
        .send()
        .await?
        .get_receipt()
        .await?;
    dbg!(r);

    Ok(())
}
