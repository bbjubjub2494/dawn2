use super::access_list::AccessList;
use crate::{keccak256, Bytes, ChainId, Signature, TxKind, TxType, B256, U256};
use alloy_rlp::{length_of_length, Decodable, Encodable, Header};
use core::mem;

#[cfg(any(test, feature = "reth-codec"))]
use reth_codecs::Compact;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;
use serde::{Deserialize, Serialize};

use dawn_crypto::Ciphertext;
use dawn_crypto::EphemeralPublicKey;

/// A transaction with a priority fee ([EIP-1559](https://eips.ethereum.org/EIPS/eip-1559)).
#[cfg_attr(any(test, feature = "reth-codec"), reth_codecs::reth_codec)]
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct TxDawnEncrypted {
    /// Added as EIP-155: Simple replay attack protection
    pub chain_id: ChainId,

    /// A scalar value equal to the number of transactions sent by the sender; formally Tn.
    pub nonce: u64,

    /// A scalar value equal to the maximum
    /// amount of gas that should be used in executing
    /// this transaction. This is paid up-front, before any
    /// computation is done and may not be increased
    /// later; formally Tg.
    pub gas_limit: u64,

    /// A scalar value equal to the maximum
    /// amount of gas that should be used in executing
    /// this transaction. This is paid up-front, before any
    /// computation is done and may not be increased
    /// later; formally Tg.
    ///
    /// As ethereum circulation is around 120mil eth as of 2022 that is around
    /// 120000000000000000000000000 wei we are safe to use u128 as its max number is:
    /// 340282366920938463463374607431768211455
    ///
    /// This is also known as `GasFeeCap`
    pub max_fee_per_gas: u128,

    /// Max Priority fee that transaction is paying
    ///
    /// As ethereum circulation is around 120mil eth as of 2022 that is around
    /// 120000000000000000000000000 wei we are safe to use u128 as its max number is:
    /// 340282366920938463463374607431768211455
    ///
    /// This is also known as `GasTipCap`
    pub max_priority_fee_per_gas: u128,

    /// A scalar value equal to the number of Wei to
    /// be transferred to the message call’s recipient or,
    /// in the case of contract creation, as an endowment
    /// to the newly created account; formally Tv.
    pub value: U256,

    /// The accessList specifies a list of addresses and storage keys;
    /// these addresses and storage keys are added into the `accessed_addresses`
    /// and `accessed_storage_keys` global sets (introduced in EIP-2929).
    /// A gas cost is charged, though at a discount relative to the cost of
    /// accessing outside the list.
    pub access_list: AccessList,

    pub ciphertext: Ciphertext,
}

impl TxDawnEncrypted {
    /// Returns the effective gas price for the given `base_fee`.
    pub const fn effective_gas_price(&self, base_fee: Option<u64>) -> u128 {
        match base_fee {
            None => self.max_fee_per_gas,
            Some(base_fee) => {
                // if the tip is greater than the max priority fee per gas, set it to the max
                // priority fee per gas + base fee
                let tip = self.max_fee_per_gas.saturating_sub(base_fee as u128);
                if tip > self.max_priority_fee_per_gas {
                    self.max_priority_fee_per_gas + base_fee as u128
                } else {
                    // otherwise return the max fee per gas
                    self.max_fee_per_gas
                }
            }
        }
    }

    /// Decodes the inner [`TxDawnEncrypted`] fields from RLP bytes.
    ///
    /// NOTE: This assumes a RLP header has already been decoded, and _just_ decodes the following
    /// RLP fields in the following order:
    ///
    /// - `chain_id`
    /// - `nonce`
    /// - `max_priority_fee_per_gas`
    /// - `max_fee_per_gas`
    /// - `gas_limit`
    /// - `to`
    /// - `value`
    /// - `data` (`input`)
    /// - `access_list`
    pub(crate) fn decode_inner(buf: &mut &[u8]) -> alloy_rlp::Result<Self> {
        Ok(Self {
            chain_id: Decodable::decode(buf)?,
            nonce: Decodable::decode(buf)?,
            max_priority_fee_per_gas: Decodable::decode(buf)?,
            max_fee_per_gas: Decodable::decode(buf)?,
            gas_limit: Decodable::decode(buf)?,
            value: Decodable::decode(buf)?,
            access_list: Decodable::decode(buf)?,
            ciphertext: Ciphertext {
                u: EphemeralPublicKey(Decodable::decode(buf)?),
                payload: Decodable::decode(buf)?,
                tag: Decodable::decode(buf)?,
            },
        })
    }

    /// Encodes only the transaction's fields into the desired buffer, without a RLP header.
    pub(crate) fn fields_len(&self) -> usize {
        self.chain_id.length() +
            self.nonce.length() +
            self.max_priority_fee_per_gas.length() +
            self.max_fee_per_gas.length() +
            self.gas_limit.length() +
            self.value.length() +
            self.access_list.length() +
         self.ciphertext.u.0.length() +
         self.ciphertext.payload.length() +
         self.ciphertext.tag.length()
    }

    /// Encodes only the transaction's fields into the desired buffer, without a RLP header.
    pub(crate) fn encode_fields(&self, out: &mut dyn bytes::BufMut) {
        self.chain_id.encode(out);
        self.nonce.encode(out);
        self.max_priority_fee_per_gas.encode(out);
        self.max_fee_per_gas.encode(out);
        self.gas_limit.encode(out);
        self.value.encode(out);
        self.access_list.encode(out);
        self.ciphertext.u.0.encode(out);
        self.ciphertext.payload.encode(out);
        self.ciphertext.tag.encode(out);
    }

    /// Inner encoding function that is used for both rlp [`Encodable`] trait and for calculating
    /// hash that for eip2718 does not require rlp header
    ///
    /// This encodes the transaction as:
    /// `rlp(chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas, gas_limit to, value, input,
    /// access_list, y_parity, r, s)`
    pub(crate) fn encode_with_signature(
        &self,
        signature: &Signature,
        out: &mut dyn bytes::BufMut,
        with_header: bool,
    ) {
        let payload_length = self.fields_len() + signature.payload_len();
        if with_header {
            Header {
                list: false,
                payload_length: 1 + length_of_length(payload_length) + payload_length,
            }
            .encode(out);
        }
        out.put_u8(self.tx_type() as u8);
        let header = Header { list: true, payload_length };
        header.encode(out);
        self.encode_fields(out);
        signature.encode(out);
    }

    /// Output the length of the RLP signed transaction encoding, _without_ a RLP string header.
    pub(crate) fn payload_len_with_signature_without_header(&self, signature: &Signature) -> usize {
        let payload_length = self.fields_len() + signature.payload_len();
        // 'transaction type byte length' + 'header length' + 'payload length'
        1 + length_of_length(payload_length) + payload_length
    }

    /// Output the length of the RLP signed transaction encoding. This encodes with a RLP header.
    pub(crate) fn payload_len_with_signature(&self, signature: &Signature) -> usize {
        let len = self.payload_len_with_signature_without_header(signature);
        length_of_length(len) + len
    }

    /// Get transaction type
    pub(crate) const fn tx_type(&self) -> TxType {
        TxType::DawnEncrypted
    }

    /// Calculates a heuristic for the in-memory size of the [`TxDawnEncrypted`] transaction.
    #[inline]
    pub fn size(&self) -> usize {
        mem::size_of::<ChainId>() + // chain_id
        mem::size_of::<u64>() + // nonce
        mem::size_of::<u64>() + // gas_limit
        mem::size_of::<u128>() + // max_fee_per_gas
        mem::size_of::<u128>() + // max_priority_fee_per_gas
        mem::size_of::<U256>() + // value
        self.access_list.size() + // access_list
        self.ciphertext.u.0.len() + // ciphertext
        self.ciphertext.payload.len() +
        self.ciphertext.tag.len()
    }

    /// Encodes the EIP-1559 transaction in RLP for signing.
    ///
    /// This encodes the transaction as:
    /// `tx_type || rlp(chain_id, nonce, max_priority_fee_per_gas, max_fee_per_gas, gas_limit, to,
    /// value, input, access_list)`
    ///
    /// Note that there is no rlp header before the transaction type byte.
    pub(crate) fn encode_for_signing(&self, out: &mut dyn bytes::BufMut) {
        out.put_u8(self.tx_type() as u8);
        Header { list: true, payload_length: self.fields_len() }.encode(out);
        self.encode_fields(out);
    }

    /// Outputs the length of the signature RLP encoding for the transaction.
    pub(crate) fn payload_len_for_signature(&self) -> usize {
        let payload_length = self.fields_len();
        // 'transaction type byte length' + 'header length' + 'payload length'
        1 + length_of_length(payload_length) + payload_length
    }

    /// Outputs the signature hash of the transaction by first encoding without a signature, then
    /// hashing.
    pub(crate) fn signature_hash(&self) -> B256 {
        let mut buf = Vec::with_capacity(self.payload_len_for_signature());
        self.encode_for_signing(&mut buf);
        keccak256(&buf)
    }
}
