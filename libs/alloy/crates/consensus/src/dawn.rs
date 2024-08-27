use alloy_primitives::*;

use crate::{
    transaction::{SignableTransaction, Transaction},
    Signed, TxDawnDecrypted, TxDawnEncrypted,
};
use dawn_crypto::{Ciphertext, DecryptionKey, MasterPublicKey};

#[derive(Debug)]
pub enum Error {
    SignatureError(SignatureError),
    AuthenticationError,
    InvalidDecryptionKey,
    InvalidSender,
    ShortPayloadError,
}

fn label(chain_id: ChainId, sender: &Address, nonce: u64) -> [u8; 96] {
    let mut label = [0; 96];
    label[24..32].copy_from_slice(&chain_id.to_be_bytes());
    label[44..64].copy_from_slice(sender.as_slice());
    label[88..96].copy_from_slice(&nonce.to_be_bytes());
    label
}

#[cfg(feature = "k256")]
pub fn decrypt(
    signed: &Signed<TxDawnEncrypted>,
    decryption_key: &DecryptionKey,
) -> Result<Signed<TxDawnDecrypted>, Error> {
    let sender = signed.recover_signer().map_err(Error::SignatureError)?;
    let tx = decrypt_unsigned(signed.tx(), decryption_key, &sender)?;
    Ok(Signed::new_unchecked(tx, *signed.signature(), *signed.hash()))
}

#[cfg(feature = "k256")]
pub fn reencrypt(signed: &Signed<TxDawnDecrypted>) -> Result<Signed<TxDawnEncrypted>, Error> {
    let tx = reencrypt_unsigned(signed.tx()).into_signed(signed.signature().clone());
    let sender = tx.recover_signer().map_err(Error::SignatureError)?;
    if sender != signed.tx().sender {
        return Err(Error::InvalidSender);
    }
    Ok(tx)
}

pub fn decrypt_unsigned(
    tx: &TxDawnEncrypted,
    decryption_key: &DecryptionKey,
    sender: &Address,
) -> Result<TxDawnDecrypted, Error> {
    let label = label(tx.chain_id, sender, tx.nonce);
    let Some(payload) = tx.ciphertext.decrypt(decryption_key, &label) else {
        return Err(Error::AuthenticationError);
    };
    if payload.len() < 20 {
        return Err(Error::ShortPayloadError);
    }
    let to = &payload[..20];
    let input = &payload[20..];
    Ok(TxDawnDecrypted {
        chain_id: tx.chain_id,
        nonce: tx.nonce,
        gas_limit: tx.gas_limit,
        max_fee_per_gas: tx.max_fee_per_gas,
        max_priority_fee_per_gas: tx.max_priority_fee_per_gas,
        to: TxKind::Call(Address::from_slice(to)),
        value: tx.value,
        access_list: tx.access_list.clone(),
        input: Bytes::copy_from_slice(input),
        ephemeral_public_key: tx.ciphertext.u.clone(),
        decryption_key: decryption_key.clone(),
        sender: *sender,
    })
}

pub fn reencrypt_unsigned(tx: &TxDawnDecrypted) -> TxDawnEncrypted {
    let label = label(tx.chain_id, &tx.sender, tx.nonce);
    let TxKind::Call(to) = tx.to() else {
        panic!("only Call transactions are supported");
    };
    let payload = [to.as_slice(), &tx.input[..]].concat();
    let ciphertext =
        Ciphertext::reencrypt(&tx.ephemeral_public_key, &tx.decryption_key, &payload, &label);
    TxDawnEncrypted {
        chain_id: tx.chain_id,
        nonce: tx.nonce,
        gas_limit: tx.gas_limit,
        max_fee_per_gas: tx.max_fee_per_gas,
        max_priority_fee_per_gas: tx.max_priority_fee_per_gas,
        value: tx.value,
        access_list: tx.access_list.clone(),
        ciphertext,
    }
}

pub fn encrypt<T: Transaction>(mpk: &MasterPublicKey, tx: &T, sender: &Address) -> TxDawnEncrypted {
    let TxKind::Call(to) = tx.to() else {
        panic!("only Call transactions are supported");
    };
    let payload = [to.as_slice(), tx.input()].concat();
    let chain_id = tx.chain_id().unwrap();
    let label = label(chain_id, sender, tx.nonce());
    let ciphertext = Ciphertext::encrypt(mpk, &label, &payload, &label);
    TxDawnEncrypted {
        chain_id,
        nonce: tx.nonce(),
        gas_limit: tx.gas_limit(),
        max_fee_per_gas: tx.max_fee_per_gas(),
        max_priority_fee_per_gas: tx.max_priority_fee_per_gas().unwrap_or(0),
        value: tx.value(),
        access_list: tx.access_list().unwrap_or(&Default::default()).clone(),
        ciphertext,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{transaction::SignableTransaction, TxEip1559};
    use alloy_eips::eip2930::AccessList;
    use alloy_network::TxSignerSync;
    use alloy_signer_local::PrivateKeySigner;
    use dawn_crypto::*;

    #[cfg(feature = "k256")]
    #[test]
    fn test_decrypt() {
        let (mpk, msk) = generate();
        let signer = PrivateKeySigner::random();
        let chain_id = 1;
        let nonce = 0;
        let value = U256::from(1_000_000_000_000_000_000);
        let label = label(chain_id, &signer.address(), nonce);
        let tx = TxDawnEncrypted {
            chain_id,
            nonce,
            gas_limit: 1_000_000,
            max_fee_per_gas: 1_000_000_000,
            max_priority_fee_per_gas: 10_000_000,
            value,
            access_list: Default::default(),
            ciphertext: Ciphertext::encrypt(&mpk, &label, &[0x22; 20], &label),
        };
        let signature = signer.sign_transaction_sync(&mut tx).unwrap();
        let signed = tx.into_signed(signature);
        let decryption_key = reveal(&label, &msk);
        let decrypted = decrypt(&signed, &decryption_key).unwrap();
        assert_eq!(decrypted.tx().chain_id, 1);
        assert_eq!(decrypted.tx().nonce, 0);
        assert_eq!(decrypted.tx().gas_limit, 0);
        assert_eq!(decrypted.tx().max_fee_per_gas, 0);
        assert_eq!(decrypted.tx().max_priority_fee_per_gas, 0);
        assert_eq!(decrypted.tx().value, value);
        assert_eq!(decrypted.tx().access_list, Default::default());
        assert_eq!(decrypted.tx().to, address!("2222222222222222222222222222222222222222").into());
        assert_eq!(&decrypted.tx().input[..], &[]);
    }

    #[test]
    fn test_decrypt_unsigned() {
        let (mpk, msk) = generate();
        let chain_id = 1;
        let nonce = 0;
        let value = U256::from(1_000_000_000_000_000_000u128);
        let gas_limit = 1_000_000;
        let max_fee_per_gas = 1_000_000_000;
        let max_priority_fee_per_gas = 10_000_000;
        let sender = address!("3333333333333333333333333333333333333333");
        let label = label(chain_id, &sender, nonce);
        let access_list = AccessList::default();
        let input = Bytes::copy_from_slice(&[]);
        let ciphertext = Ciphertext::encrypt(&mpk, &label, &[0x22; 20], &label);
        let tx = TxDawnEncrypted {
            chain_id,
            nonce,
            gas_limit,
            max_fee_per_gas,
            max_priority_fee_per_gas,
            value,
            access_list: access_list.clone(),
            ciphertext,
        };
        let decryption_key = reveal(&label, &msk);
        let decrypted = decrypt_unsigned(&tx, &decryption_key, &sender).unwrap();
        assert_eq!(
            decrypted,
            TxDawnDecrypted {
                chain_id,
                nonce,
                gas_limit,
                max_fee_per_gas,
                max_priority_fee_per_gas,
                value,
                access_list,
                to: address!("2222222222222222222222222222222222222222").into(),
                input,
                sender,
                ephemeral_public_key: tx.ciphertext.u.clone(),
                decryption_key,
            }
        );
    }

    #[test]
    fn test_encrypt() {
        let (mpk, msk) = generate();
        let chain_id = 1;
        let nonce = 0;
        let value = U256::from(1_000_000_000_000_000_000u128);
        let gas_limit = 1_000_000;
        let max_fee_per_gas = 1_000_000_000;
        let max_priority_fee_per_gas = 10_000_000;
        let sender = address!("3333333333333333333333333333333333333333");
        let label = label(chain_id, &sender, nonce);
        let access_list = AccessList::default();
        let input = Bytes::copy_from_slice(b"hello");
        let tx = encrypt(
            &mpk,
            &TxEip1559 {
                chain_id,
                nonce,
                gas_limit,
                max_fee_per_gas,
                max_priority_fee_per_gas,
                value,
                access_list: access_list.clone(),
                to: address!("2222222222222222222222222222222222222222").into(),
                input: input.clone(),
            },
            &sender,
        );
        let decryption_key = reveal(&label, &msk);
        let decrypted = decrypt_unsigned(&tx, &decryption_key, &sender).unwrap();
        assert_eq!(
            decrypted,
            TxDawnDecrypted {
                chain_id,
                nonce,
                gas_limit,
                max_fee_per_gas,
                max_priority_fee_per_gas,
                value,
                access_list,
                to: address!("2222222222222222222222222222222222222222").into(),
                input,
                sender,
                ephemeral_public_key: tx.ciphertext.u.clone(),
                decryption_key,
            }
        );
    }
}
