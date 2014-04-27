#!/bin/bash

cd $(dirname $0)
PATH=$PWD/../../_compiled/bin-tests:$PWD/../../_compiled/bin:$PATH

trap 'kill $attacker' EXIT

attacker-gw & attacker=$!
attack-scenario
