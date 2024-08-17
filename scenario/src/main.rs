use alloy::network::EthereumWallet;
use alloy::primitives::*;
use alloy::providers::{Provider, ProviderBuilder};
use alloy::signers::local::{coins_bip39::English, MnemonicBuilder};
use alloy::sol;
use eyre::Result;

sol!(
    #[allow(missing_docs)]
    #[sol(rpc)]
    OvercollateralizedAuctions,
    "../contracts/out/OvercollateralizedAuctions.sol/OvercollateralizedAuctions.json"
);

fn derive_key(index: u32) -> Result<EthereumWallet> {
    let phrase = "test test test test test test test test test test test junk"; // reth default mnemonic
    let signer = MnemonicBuilder::<English>::default()
        .phrase(phrase)
        .index(index)?
        .build()?;
    Ok(EthereumWallet::from(signer))
}

#[tokio::main]
async fn main() -> Result<()> {
    let rpc_url = std::env::var("ETH_RPC_URL")?.parse()?;
    let deployer_wallet = derive_key(0)?;
    let provider = ProviderBuilder::new()
        .with_recommended_fillers()
        .wallet(deployer_wallet)
        .on_http(rpc_url);

    // Deploy the contract.
    let block_delay = 2;
    let auctions = OvercollateralizedAuctions::deploy(provider, block_delay).await?;
    dbg!(auctions.address());

    Ok(())
}
