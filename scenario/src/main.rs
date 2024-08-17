use alloy::providers::{Provider, ProviderBuilder};
use eyre::Result;

#[tokio::main]
async fn main() -> Result<()> {
    let rpc_url = std::env::var("ETH_RPC_URL")?.parse()?;
    let provider = ProviderBuilder::new().on_http(rpc_url);

    let latest_block = provider.get_block_number().await?;

    // Print the block number.
    println!("Latest block number: {latest_block}");

    Ok(())
}
