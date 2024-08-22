use ic_bls12_381::{
    hash_to_curve::{ExpandMsgXmd, HashToCurve},
    G1Affine, G1Projective,
};

const DOMAIN: &[u8] = b"BLS_SIG_BLS12381G1_XMD:SHA-256_SSWU_RO_NUL_";

pub fn hash_to_g1(label: &[u8]) -> G1Affine {
    let g: G1Projective = HashToCurve::<ExpandMsgXmd<sha2::Sha256>>::hash_to_curve(label, DOMAIN);
    g.into()
}
