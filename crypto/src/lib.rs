mod encapsulate;
mod hash_to_g1;
mod symmetric;

use ark_bls12_381::{Fq12, G1Affine, G2Affine, G2Projective};
use ark_serialize::CanonicalSerialize;

use sha2::{Digest, Sha256};

pub use encapsulate::*;

pub struct Ciphertext {
    u: G2Affine,
    payload: Vec<u8>,
    tag: [u8; symmetric::TAG_SIZE],
}

impl Ciphertext {
    pub fn encrypt(
        ek: G2Projective,
        label: &[u8],
        payload: &[u8],
        associated_data: &[u8],
    ) -> Result<Self, hash_to_g1::Error> {
        let mut buf = payload.to_vec();
        let (u, s) = encapsulate::share(label, ek)?;
        let k = derive_key(s);
        let tag = symmetric::encrypt(&k, &mut buf, associated_data);
        Ok(Self {
            u,
            payload: buf,
            tag,
        })
    }

    pub fn decrypt(
        self,
        σ: G1Affine,
        associated_data: &[u8],
    ) -> Result<Vec<u8>, chacha20poly1305::Error> {
        let mut buf = self.payload.to_vec();
        let s = encapsulate::recover(self.u, σ);
        let k = derive_key(s);
        symmetric::decrypt(&k, &mut buf, associated_data, &self.tag)?;
        Ok(buf)
    }
}

fn derive_key(s: Fq12) -> [u8; symmetric::KEY_SIZE] {
    let mut k = [0u8; symmetric::KEY_SIZE];
    let mut hasher = Sha256::new();
    s.serialize_compressed(&mut hasher)
        .expect("serialization failed");
    k.copy_from_slice(hasher.finalize().as_slice());
    k
}

#[cfg(test)]
mod tests {
    use super::*;

    use ark_bls12_381::Fr;
    use ark_ec::AffineRepr;
    use ark_std::UniformRand;
    use rand::rngs::OsRng;

    #[test]
    fn test_encrypt() {
        let label = b"test";
        let sk = Fr::rand(&mut OsRng);
        let ek = G2Affine::generator() * sk;
        let payload = b"hello world";
        let ad = b"associated data";
        let c = Ciphertext::encrypt(ek, label, payload, ad).unwrap();
        let σ = encapsulate::reveal(label, sk).unwrap();
        let decrypted = c.decrypt(σ, ad).unwrap();
        assert_eq!(decrypted, payload);
    }
}
