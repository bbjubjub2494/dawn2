use crate::hash_to_g1;
use group::Group;
use ic_bls12_381::multi_miller_loop;
use ic_bls12_381::pairing;
use ic_bls12_381::{G1Affine, G2Affine, G2Prepared, Gt, Scalar};

use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct MasterPublicKey([u8; 96]);

impl MasterPublicKey {
    pub fn unpack(&self) -> G2Affine {
        G2Affine::from_compressed(&self.0).unwrap()
    }
    pub fn pack(e: &G2Affine) -> Self {
        Self(e.to_compressed())
    }
}

#[derive(Debug)]
pub struct MasterPrivateKey(Scalar);

impl MasterPrivateKey {
    pub fn to_bytes(&self) -> [u8; 32] {
        self.0.to_bytes()
    }

    pub fn from_bytes(bytes: [u8; 32]) -> Self {
        Self(Scalar::from_bytes(&bytes).unwrap())
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct EphemeralPublicKey([u8; 96]);

impl EphemeralPublicKey {
    pub fn unpack(&self) -> G2Affine {
        G2Affine::from_compressed(&self.0).unwrap()
    }
    pub fn pack(e: &G2Affine) -> Self {
        Self(e.to_compressed())
    }
}

#[derive(Debug, PartialEq, Eq)]
pub struct SharedSecret(Gt);

impl SharedSecret {
    pub fn to_bytes(&self) -> [u8; 576] {
        self.0.to_bytes()
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct DecryptionKey([u8; 48]);

impl DecryptionKey {
    pub fn unpack(&self) -> G1Affine {
        G1Affine::from_compressed(&self.0).unwrap()
    }
    pub fn pack(e: &G1Affine) -> Self {
        Self(e.to_compressed())
    }
}

pub fn generate() -> (MasterPublicKey, MasterPrivateKey) {
    let sk = random_scalar();
    let pk = G2Affine::generator() * sk;
    (MasterPublicKey::pack(&pk.into()), MasterPrivateKey(sk))
}

pub fn share(label: &[u8], mpk: &MasterPublicKey) -> (EphemeralPublicKey, SharedSecret) {
    let r = random_scalar();
    let u: G2Affine = (r * G2Affine::generator()).into();
    let s = pairing(&hash_to_g1::hash_to_g1(label), &mpk.unpack()) * r;
    (EphemeralPublicKey::pack(&u.into()), SharedSecret(s))
}

pub fn reveal(label: &[u8], sk: &MasterPrivateKey) -> DecryptionKey {
    let dk = hash_to_g1::hash_to_g1(label) * sk.0;
    DecryptionKey::pack(&dk.into())
}

pub fn recover(u: &EphemeralPublicKey, dk: &DecryptionKey) -> SharedSecret {
    SharedSecret(pairing(&dk.unpack(), &u.unpack()))
}

pub fn verify(label: &[u8], mpk: &MasterPublicKey, dk: &DecryptionKey) -> bool {
    fast_pairing_equality(
        &hash_to_g1::hash_to_g1(label),
        &mpk.unpack(),
        &dk.unpack(),
        &G2Affine::generator(),
    )
}

// yoinked from https://github.com/noislabs/drand-verify
/// Checks if e(p, q) == e(r, s)
///
/// See https://hackmd.io/@benjaminion/bls12-381#Final-exponentiation.
///
/// Optimized by this trick:
///   Instead of doing e(a,b) (in G2) multiplied by e(-c,d) (in G2)
///   (which is costly is to multiply in G2 because these are very big numbers)
///   we can do FinalExponentiation(MillerLoop( [a,b], [-c,d] )) which is the same
///   in an optimized way.
fn fast_pairing_equality(p: &G1Affine, q: &G2Affine, r: &G1Affine, s: &G2Affine) -> bool {
    let minus_p = -p;
    // "some number of (G1, G2) pairs" are the inputs of the miller loop
    let pair1 = (&minus_p, &G2Prepared::from(*q));
    let pair2 = (r, &G2Prepared::from(*s));
    let looped = multi_miller_loop(&[pair1, pair2]);
    let value = looped.final_exponentiation();
    value.is_identity().into()
}

#[cfg(feature = "mesalock_sgx")]
fn random_scalar() -> Scalar {
    let rng = sgx_rand::os::SgxRng::new().unwrap();
    Scalar::random(rng)
}
#[cfg(feature = "no_mesalock_sgx")]
fn random_scalar() -> Scalar {
    use group::ff::Field;
    let rng = rand::rngs::OsRng;
    Scalar::random(rng)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_encapsulate_reveal() {
        let label = b"test";
        let (mpk, msk) = generate();
        let (u, s) = share(label, &mpk);
        let dk = reveal(label, &msk);
        let s_prime = recover(&u, &dk);
        assert_eq!(s, s_prime);
    }

    #[test]
    fn test_verify() {
        let label = b"test";
        let (mpk, msk) = generate();
        let dk = reveal(label, &msk);
        assert!(verify(label, &mpk, &dk));
    }
}
