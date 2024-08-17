set -euxo pipefail

cleanup_tempdir() {
	rm -rf "$tempdir"
}

setup_tmpdir() {
	if [[ -v tempdir ]]; then
		return
	fi
	tempdir=$(mktemp -dt f3b.XXXXXX)
	trap cleanup_tempdir EXIT
}

# reth default mnemonic
mnemonic="test test test test test test test test test test test junk"


RETH="$PWD/reth/target/debug/reth"
reth_args=()

run_reth() {
	command "$RETH" "${reth_args[@]}" "$@"
}

SCENARIO="$PWD/scenario/target/debug/scenario"
scenario_args=()
run_scenario() {
	command "$SCENARIO" "${scenario_args[@]}" "$@"
}

export ETH_RPC_URL=http://localhost:8545
