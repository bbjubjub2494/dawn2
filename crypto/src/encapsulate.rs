use crate::hash_to_g1;
use ark_bls12_381::{Bls12_381, Fq12, Fr, G1Affine, G2Affine, G2Projective};
use ark_ec::pairing::Pairing;
use ark_ec::AffineRepr;
use ark_std::{UniformRand, Zero};

use rand::rngs::OsRng;

pub fn share(label: &[u8], ek: G2Projective) -> Result<(G2Affine, Fq12), hash_to_g1::Error> {
    let r = ark_bls12_381::Fr::rand(&mut OsRng);
    let u = G2Affine::generator() * r;
    let s = Bls12_381::pairing(hash_to_g1::hash_to_g1(label)?.into_group(), ek) * r;
    Ok((u.into(), s.0))
}

pub fn reveal(label: &[u8], sk: Fr) -> Result<G1Affine, hash_to_g1::Error> {
    let σ = hash_to_g1::hash_to_g1(label)? * sk;
    Ok(σ.into())
}

pub fn recover(u: G2Affine, σ: G1Affine) -> Fq12 {
    let s = Bls12_381::pairing(σ, u);
    s.0
}

pub fn verify(label: &[u8], ek: G2Projective, σ: G1Affine) -> Result<bool, hash_to_g1::Error> {
    let r = Bls12_381::multi_pairing(
        [hash_to_g1::hash_to_g1(label)?.into_group(), σ.into()],
        [ek, -G2Affine::generator().into_group()],
    );
    Ok(r.is_zero())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_encapsulate_reveal() -> Result<(), hash_to_g1::Error> {
        let label = b"test";
        let sk = Fr::rand(&mut OsRng);
        let ek = G2Affine::generator() * sk;
        let (u, s) = share(label, ek)?;
        let σ = reveal(label, sk)?;
        let s_prime = recover(u, σ);
        assert_eq!(s, s_prime);
        Ok(())
    }

    #[test]
    fn test_verify() -> Result<(), hash_to_g1::Error> {
        let label = b"test";
        let sk = Fr::rand(&mut OsRng);
        let ek = G2Affine::generator() * sk;
        let σ = reveal(label, sk)?;
        assert!(verify(label, ek, σ)?);
        Ok(())
    }
}
