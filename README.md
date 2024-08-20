# Delayed Execution Ethereum

Dawn is a prototype that demonstrates temporary encryption as a built-in feature of smart contract blockchains.
The motivation is to provide smart contract developers with stronger assumptions
so that features such as frontrunning protection and sealed-bid auctions become much simpler to implement.

Traditionally, an interaction involves the user submitting arbitrary data to the blockchain as input for a smart contract.
There is no guarantee that a transaction cannot be inserted before the original transaction with knowledge of the input.
This allows frontrunning attacks, and more generally introduces information asymmetries in favor of later-moving parties,
which complicate a number of higher-level protocols such as voting, randomness generation, and sealed-bid auctions.
(See [Bet24] §2.4 and §3.1 for treatment of those protocols)

In Dawn, users encrypt their transactions so that a threshold network of trusted execution environments can decrypt them.
The transactions are included in the blockchain, but their execution is delayed until they are canonical.
Then, the threshold nodes, upon learning that a transaction is canonical using a light client, must decrypt it.
They can then be executed in the next block.

# Design

TODO subject to change, implement

- SMC: secret management committee, the threshold network
- CG: consensus group, the set of nodes that determine which blocks are canonical.

We introduce a new Ethereum hard fork after which blocks also contain a shadow block.
The shadow block is a list of transactions whose total gas does not exceed the gas limit, and a beneficiary address.
The block producer is no longer able to choose which transactions to include in their block:
instead, they must use the transactions from the dth ancestor of their block.
However, they can create the shadow block for d blocks in the future and collect the fees.

We also introduce new transaction types.
The first one is an encrypted transaction:
its `to` and `data` fields are temporarily encrypted.
The second one is a decrypted transaction:
it comes from an encrypted transaction and must be re-encrypted to check the signature.
The third one is an undecrypted transaction:
it results from a faulty encrypted transaction, and is only valid if the encrypted data it contains fails to decrypt.
If that is the case, it can be executed on the blockchain and the sender of the transaction will pay gas in proportion.

The SMC runs distributed key generation and produces a point on G2 of BLS-12-381 as its public key.
To encrypt, users perform IBE key encapsulation as in [Bet23] §3.1,
except that the identity label is a concatenation of the chain id, sender address, and account nonce.
The decryption key is included in (un)decrypted transactions as evidence that the SMC acted correctly.

For symmetric encryption, we use ChaCha20-Poly1305, with the identity label included as the associated data.
This is defense in-depth against copy attacks.

# Implementation

`reth/` is a fork of [Reth] modified to support encryption and delayed execution.
In the prototype, it is used as a single-node proof-of-authority blockchain.

`contracts/` contains EVM smart contracts to perform auctions.

`scenario/` contains Rust scripts that simulate an auction with many bidders.

`sgx/` performs SMC duties inside of an Intel SGX enclave.

Dependencies are [Foundry], [Cargo], OpenSSL (due to a Reth dependency).
[Just] can be used to run the demo.

# Copying

All work is dual-licensed MIT and Apache 2:
Reth by its contributors, the contracts by DEDIS, and the rest by me.

[Bet23]: https://www.epfl.ch/labs/dedis/wp-content/uploads/2024/06/Bettens2023_FrontRunningProtection.pdf
[Bet24]: https://blog.bbjubjub.fr/thesis.pdf
[Reth]: https://github.com/paradigmxyz/reth/
[Foundry]: https://getfoundry.sh/
[Cargo]: https://crates.io/
[Just]: https://just.systems/
