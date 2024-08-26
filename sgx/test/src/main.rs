use std::process::{Command, Stdio};

use dawn_crypto::verify;
use dawn_enclave_protocol::{Request, Response};

fn main() {
    let mut cmd = Command::new("./app")
        .current_dir("../bin")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .spawn()
        .unwrap();
    let stdin = cmd.stdin.take().unwrap();
    let stdout = cmd.stdout.take().unwrap();
    let request: Request = Request::Generate();
    serde_cbor::to_writer(stdin, &request).unwrap();
    let response: Response = serde_cbor::from_reader(stdout).unwrap();
    dbg!(cmd.wait().unwrap());
    let Response::Generate(mpk, emsk) = response else { panic!("Expected Generate response") };
    drop(cmd);

    let mut cmd = Command::new("./app")
        .current_dir("../bin")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .spawn()
        .unwrap();
    let stdin = cmd.stdin.take().unwrap();
    let stdout = cmd.stdout.take().unwrap();
    let request: Request = Request::Reveal(b"label".to_vec(), emsk);
    serde_cbor::to_writer(stdin, &request).unwrap();
    let response: Response = serde_cbor::from_reader(stdout).unwrap();
    dbg!(cmd.wait().unwrap());
    let Response::Reveal(dk) = response else { panic!("Expected Reveal response") };

    assert!(verify(b"label", &mpk, &dk));
}
