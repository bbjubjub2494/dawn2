#![cfg_attr(feature = "mesalock_sgx", no_std)]
#![cfg_attr(
    all(target_env = "sgx", target_vendor = "mesalock"),
    feature(rustc_private)
)]

#[cfg(feature = "mesalock_sgx")]
extern crate sgx_tstd as std;

use serde::{Deserialize, Serialize};

use std::vec::Vec;

pub use dawn_crypto::{DecryptionKey, EphemeralPublicKey, MasterPublicKey};

pub type Label = Vec<u8>;

#[derive(Debug, Serialize, Deserialize)]
pub struct SealedMasterPrivateKey(pub Vec<u8>);

#[derive(Debug, Serialize, Deserialize)]
pub enum Request {
    Generate(),
    Reveal(Label, SealedMasterPrivateKey),
}

#[derive(Debug, Serialize, Deserialize)]
pub enum Response {
    Generate(MasterPublicKey, SealedMasterPrivateKey),
    Reveal(DecryptionKey),
}
