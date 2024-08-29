// Copyright (C) 2017-2018 Baidu, Inc. All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//  * Neither the name of Baidu, Inc., nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

extern crate dawn_crypto;
extern crate dawn_enclave_protocol;
extern crate sgx_types;
extern crate sgx_urts;
use sgx_types::*;
use sgx_urts::SgxEnclave;

use std::io;
use std::process::{Command, Stdio};

use dawn_crypto::verify;
use dawn_enclave_protocol::{Request, Response};

static ENCLAVE_FILE: &'static str = "enclave.signed.so";

extern "C" {
    fn handle(eid: sgx_enclave_id_t, retval: *mut sgx_status_t) -> sgx_status_t;
}

fn init_enclave() -> SgxResult<SgxEnclave> {
    let mut launch_token: sgx_launch_token_t = [0; 1024];
    let mut launch_token_updated: i32 = 0;

    // Step 2: call sgx_create_enclave to initialize an enclave instance
    // Debug Support: set 2nd parameter to 1
    let debug = 1;
    let mut misc_attr = sgx_misc_attribute_t {
        secs_attr: sgx_attributes_t { flags: 0, xfrm: 0 },
        misc_select: 0,
    };
    let enclave = SgxEnclave::create(
        ENCLAVE_FILE,
        debug,
        &mut launch_token,
        &mut launch_token_updated,
        &mut misc_attr,
    )?;

    Ok(enclave)
}

fn enclave_handle(request: Request) -> io::Result<Response> {
    let mut cmd = Command::new("./app")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .spawn()?;
    let stdin = cmd.stdin.take().unwrap();
    let stdout = cmd.stdout.take().unwrap();
    serde_cbor::to_writer(stdin, &request).unwrap();
    let response: Response = serde_cbor::from_reader(stdout).unwrap();
    if !cmd.wait()?.success() {
        panic!("Enclave failed to run");
    }
    Ok(response)
}

fn selfcheck() -> io::Result<()> {
    let request = Request::Generate();
    let Response::Generate(mpk, emsk) = enclave_handle(request)? else { panic!("Expected Generate response") };

    let request = Request::Reveal(b"label".to_vec(), emsk);
    let Response::Reveal(dk) = enclave_handle(request)? else { panic!("Expected Reveal response") };

    assert!(verify(b"label", &mpk, &dk));
    Ok(())
}

fn generate() -> io::Result<()> {
    let request = Request::Generate();
    let Response::Generate(mpk, emsk) = enclave_handle(request)? else { panic!("Expected Generate response") };

    serde_json::to_writer(std::io::stdout(), &(mpk, emsk))?;
    Ok(())
}

fn main() -> io::Result<()> {
    let mut args = std::env::args();
    args.next(); // skip program name
    match args.next().as_deref() {
        Some("selfcheck") => selfcheck(),
        Some("generate") => generate(),
        Some(cmd) => Err(io::Error::new(
            io::ErrorKind::Other,
            format!("Unknown command: {}", cmd),
        )),
        None => run_enclave(),
    }
}

fn run_enclave() -> io::Result<()> {
    let enclave = match init_enclave() {
        Ok(r) => r,
        Err(x) => {
            eprintln!("[-] Init Enclave Failed {}!", x.as_str());
            std::process::exit(x as i32);
        }
    };

    let mut retval = sgx_status_t::SGX_SUCCESS;

    let result = unsafe { handle(enclave.geteid(), &mut retval) };

    match result {
        sgx_status_t::SGX_SUCCESS => {}
        _ => {
            return Err(io::Error::new(io::ErrorKind::Other, "ECALL Enclave Failed"));
        }
    }

    enclave.destroy();

    Ok(())
}
