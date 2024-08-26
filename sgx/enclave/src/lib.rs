// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License..

#![crate_name = "dawn_sgx_enclave"]
#![crate_type = "staticlib"]
#![no_std]
#![cfg_attr(target_env = "sgx", feature(rustc_private))]

extern crate sgx_tseal;
extern crate sgx_types;
#[cfg(not(target_env = "sgx"))]
#[macro_use]
extern crate sgx_tstd as std;
extern crate dawn_enclave_protocol;
use dawn_enclave_protocol::{Request, Response};
use sgx_tseal::SgxSealedData;
use sgx_types::*;
use std::io::{self, Write};
use std::vec::Vec;

#[no_mangle]
pub extern "C" fn handle() -> sgx_status_t {
    let request: Request = serde_cbor::from_reader(io::stdin()).unwrap();
    let response = match request {
        Request::Generate() => {
            let (mpk, msk) = dawn_crypto::generate();
            let aad = b"";
            let data = msk.to_bytes();
            let sealed_data = SgxSealedData::<[u8; 32]>::seal_data(aad, &data).unwrap();
            let raw = to_raw_sealed_data(&sealed_data);
            Response::Generate(mpk, dawn_enclave_protocol::SealedMasterPrivateKey(raw))
        }
        Request::Reveal(label, mut smpk) => {
            let sealed_data = from_raw_sealed_data(&mut smpk.0).unwrap();
            let msk = dawn_crypto::MasterPrivateKey::from_bytes(
                *sealed_data.unseal_data().unwrap().decrypt,
            );
            let dk = dawn_crypto::reveal(&label, &msk);
            Response::Reveal(dk)
        }
    };
    let mut stdout = io::stdout().lock();
    serde_cbor::to_writer(&mut stdout, &response).unwrap();
    stdout.flush().unwrap();

    sgx_status_t::SGX_SUCCESS
}

fn to_raw_sealed_data(sealed_data: &SgxSealedData<[u8; 32]>) -> Vec<u8> {
    let len = SgxSealedData::<[u8; 32]>::calc_raw_sealed_data_size(
        sealed_data.get_add_mac_txt_len(),
        sealed_data.get_encrypt_txt_len(),
    );
    let mut buf = vec![0; len as usize];
    unsafe {
        sealed_data
            .to_raw_sealed_data_t(buf.as_mut_ptr() as *mut sgx_sealed_data_t, len)
            .unwrap();
    }
    buf
}

fn from_raw_sealed_data(raw: &mut [u8]) -> Option<SgxSealedData<[u8; 32]>> {
    unsafe {
        SgxSealedData::<[u8; 32]>::from_raw_sealed_data_t(
            raw.as_mut_ptr() as *mut sgx_sealed_data_t,
            raw.len() as u32,
        )
    }
}
