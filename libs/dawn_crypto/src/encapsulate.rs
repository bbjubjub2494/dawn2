use crate::hash_to_g1;
use bls12_381::multi_miller_loop;
use bls12_381::pairing;
use bls12_381::{G1Affine, G2Affine, G2Prepared, G2Projective, Gt, Scalar};
use ff::Field;
use group::Group;

use rand::rngs::OsRng;

pub fn share(label: &[u8], ek: G2Projective) -> (G2Affine, Gt) {
    let r = Scalar::random(&mut OsRng);
    let u = G2Affine::generator() * r;
    let s = pairing(&hash_to_g1::hash_to_g1(label), &ek.into()) * r;
    (u.into(), s)
}

pub fn reveal(label: &[u8], sk: Scalar) -> G1Affine {
    let σ = hash_to_g1::hash_to_g1(label) * sk;
    σ.into()
}

pub fn recover(u: &G2Affine, σ: &G1Affine) -> Gt {
    pairing(σ, u)
}

pub fn verify(label: &[u8], ek: G2Projective, σ: G1Affine) -> bool {
    fast_pairing_equality(
        &hash_to_g1::hash_to_g1(label),
        &ek.into(),
        &σ,
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
        let sk = Scalar::random(&mut OsRng);
        let ek = G2Affine::generator() * sk;
        let (u, s) = share(label, ek);
        let σ = reveal(label, sk);
        let s_prime = recover(&u, &σ);
        assert_eq!(s, s_prime);
    }

    #[test]
    fn test_verify() {
        let label = b"test";
        let sk = Scalar::random(&mut OsRng);
        let ek = G2Affine::generator() * sk;
        let σ = reveal(label, sk);
        assert!(verify(label, ek, σ));
    }
}
