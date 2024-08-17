run: build-reth build-scenario
  #!/usr/bin/env bash
  
  . lib.sh
  setup_tmpdir

  tmux set -g remain-on-exit failed
  
  producer_datadir=$tempdir/producer
  mkdir -p $producer_datadir
  
  tmux splitw -hd "$RETH" -vvv node --datadir "$producer_datadir" --dev &
  
  # wait for geth to start
  while ! cast block-number 2> /dev/null; do
  	sleep 1
  done

  run_scenario

build-scenario: build-contracts
  cd scenario && cargo build

build-reth:
  cd reth && cargo build

build-contracts:
  cd contracts && forge build
