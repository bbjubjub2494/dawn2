mod encapsulate;
mod hash_to_g1;
mod symmetric;

use sha2::{Digest, Sha256};

pub use encapsulate::*;

pub struct Ciphertext {
    u: EpheremalPublicKey,
    payload: Vec<u8>,
    tag: [u8; symmetric::TAG_SIZE],
}

impl Ciphertext {
    pub fn encrypt(
        mpk: &MasterPublicKey,
        label: &[u8],
        payload: &[u8],
        associated_data: &[u8],
    ) -> Self {
        let mut buf = payload.to_vec();
        let (u, s) = encapsulate::share(label, mpk);
        let k = derive_key(&s);
        let tag = symmetric::encrypt(&k, &mut buf, associated_data);
        Self {
            u,
            payload: buf,
            tag,
        }
    }

    // decrypt the ciphertext with the given decryption key and associated data.
    // will return None if the ciphertext fails authentication.
    pub fn decrypt(&self, dk: &DecryptionKey, associated_data: &[u8]) -> Option<Vec<u8>> {
        let mut buf = self.payload.to_vec();
        let s = encapsulate::recover(&self.u, dk);
        let k = derive_key(&s);
        match symmetric::decrypt(&k, &mut buf, associated_data, &self.tag) {
            Ok(()) => Some(buf),
            Err(_) => None,
        }
    }
}

fn derive_key(s: &SharedSecret) -> [u8; symmetric::KEY_SIZE] {
    let mut k = [0u8; symmetric::KEY_SIZE];
    let mut hasher = Sha256::new();
    hasher.update(s.to_bytes());
    k.copy_from_slice(hasher.finalize().as_slice());
    k
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_encrypt() {
        let label = b"test";
        let (mpk, msk) = generate();
        let payload = b"hello world";
        let ad = b"associated data";
        let c = Ciphertext::encrypt(&mpk, label, payload, ad);
        let dk = encapsulate::reveal(label, &msk);
        let decrypted = c.decrypt(&dk, ad).unwrap();
        assert_eq!(decrypted, payload);
    }
}
