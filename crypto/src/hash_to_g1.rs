use ark_bls12_381::{g1, G1Affine, G1Projective};
use ark_ec::hashing::{
    curve_maps::wb::WBMap, map_to_curve_hasher::MapToCurveBasedHasher, HashToCurve,
    HashToCurveError,
};
use ark_ff::field_hashers::DefaultFieldHasher;

pub type Error = HashToCurveError;

const G1_DOMAIN: &[u8] = b"BLS_SIG_BLS12381G1_XMD:SHA-256_SSWU_RO_NUL_";

pub fn hash_to_g1(label: &[u8]) -> Result<G1Affine, Error> {
    let dst = G1_DOMAIN;
    let mapper = MapToCurveBasedHasher::<
        G1Projective,
        DefaultFieldHasher<sha2::Sha256, 128>,
        WBMap<g1::Config>,
    >::new(dst)?;
    mapper.hash(label)
}
