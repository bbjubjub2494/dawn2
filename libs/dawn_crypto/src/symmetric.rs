//! single-use AEAD cipher

use chacha20poly1305::{
    aead::{AeadInPlace, KeyInit},
    ChaCha20Poly1305, Nonce,
};

pub const KEY_SIZE: usize = 32;
pub const TAG_SIZE: usize = 16;

pub fn encrypt(key: &[u8; KEY_SIZE], buf: &mut [u8], associated_data: &[u8]) -> [u8; TAG_SIZE] {
    let nonce = Nonce::from_slice(&[0u8; 12]);
    let cipher = ChaCha20Poly1305::new(key.into());
    let tag = cipher
        .encrypt_in_place_detached(nonce, associated_data, buf)
        .unwrap();
    tag.into()
}

pub fn decrypt(
    key: &[u8; KEY_SIZE],
    buf: &mut [u8],
    associated_data: &[u8],
    tag: &[u8; TAG_SIZE],
) -> Result<(), chacha20poly1305::Error> {
    let nonce = Nonce::from_slice(&[0u8; 12]);
    let cipher = ChaCha20Poly1305::new(key.into());
    cipher.decrypt_in_place_detached(nonce, associated_data, buf, tag.into())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[cfg(feature = "std")]
    fn roundtrip() {
        use rand::{thread_rng, Rng};
        let mut rng = thread_rng();
        let key = rng.gen();
        let mut buf = [0; 40];
        rng.fill(&mut buf[..]);
        let mut associated_data = [0; 40];
        rng.fill(&mut associated_data[..]);

        let tag = encrypt(&key, &mut buf, &associated_data);
        assert_eq!(decrypt(&key, &mut buf, &associated_data, &tag), Ok(()));
    }
}
