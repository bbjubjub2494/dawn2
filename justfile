run: build-reth build-scenario
  #!/usr/bin/env bash
  
  . libs/lib.sh
  setup_tmpdir

  tmux set -g remain-on-exit failed
  
  producer_datadir=$tempdir/producer
  mkdir -p $producer_datadir
  
  tmux splitw -hd "$RETH" -vvv node --datadir "$producer_datadir" --dev --ws --dev.block-time 1s

  # wait for geth to start
  while ! cast block-number 2> /dev/null; do
  	sleep 1
  done

  run_scenario

reth *args: build-reth
  #!/usr/bin/env bash
  . libs/lib.sh
  run_reth {{args}}

build-scenario: build-contracts
  cd scenario && cargo build

build-reth:
  cd reth && cargo build

build-contracts:
  cd contracts && forge build

build-sgx:
  cd sgx && make

run-sgx: build-sgx
  cd sgx/bin && ./app
