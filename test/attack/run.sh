#!/bin/bash

cd $(dirname $0)
PATH=$PWD/../../_compiled/bin-tests:$PWD/../../_compiled/bin:$PATH

vcan() {(
	set -x
	sudo ip link add $1 type vcan
	sudo ip link set $1 up
)}

[ -e /sys/class/net/vcani ] || vcan vcani
[ -e /sys/class/net/vcanj ] || vcan vcanj

trap 'kill $attacker' EXIT

attacker-gw & attacker=$!
attack-scenario
