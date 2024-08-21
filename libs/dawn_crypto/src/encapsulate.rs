use crate::hash_to_g1;
use bls12_381::multi_miller_loop;
use bls12_381::pairing;
use bls12_381::{G1Affine, G2Affine, G2Prepared, Gt, Scalar};
use ff::Field;
use group::Group;

use rand::rngs::OsRng;

#[derive(Debug)]
pub struct MasterPublicKey(G2Affine);
#[derive(Debug)]
pub struct MasterPrivateKey(Scalar);
#[derive(Debug)]
pub struct EpheremalPublicKey(G2Affine);
#[derive(Debug, PartialEq, Eq)]
pub struct SharedSecret(Gt);
#[derive(Debug)]
pub struct DecryptionKey(G1Affine);

impl SharedSecret {
    pub fn to_bytes(&self) -> Vec<u8> {
        // FIXME: bls12_381 does not expose a way to serialize Gt
        // We use the debug output but this is idiosyncratic
        format!("{}", self.0).into()
    }
}

pub fn generate() -> (MasterPublicKey, MasterPrivateKey) {
    let sk = Scalar::random(&mut OsRng);
    let pk = G2Affine::generator() * sk;
    (MasterPublicKey(pk.into()), MasterPrivateKey(sk))
}

pub fn share(label: &[u8], mpk: &MasterPublicKey) -> (EpheremalPublicKey, SharedSecret) {
    let r = Scalar::random(&mut OsRng);
    let u = r * G2Affine::generator();
    let s = pairing(&hash_to_g1::hash_to_g1(label), &mpk.0) * r;
    (EpheremalPublicKey(u.into()), SharedSecret(s))
}

pub fn reveal(label: &[u8], sk: &MasterPrivateKey) -> DecryptionKey {
    let dk = hash_to_g1::hash_to_g1(label) * sk.0;
    DecryptionKey(dk.into())
}

pub fn recover(u: &EpheremalPublicKey, dk: &DecryptionKey) -> SharedSecret {
    SharedSecret(pairing(&dk.0, &u.0))
}

pub fn verify(label: &[u8], mpk: &MasterPublicKey, dk: &DecryptionKey) -> bool {
    fast_pairing_equality(
        &hash_to_g1::hash_to_g1(label),
        &mpk.0,
        &dk.0,
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
