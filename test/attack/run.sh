#!/bin/bash

cd $(dirname $0)

. ../wvtest.sh

vcan() {(
	set -x
	sudo ip link add $1 type vcan
	sudo ip link set $1 up
)}

WVSTART Attack scenario

[ -e /sys/class/net/vcani ] || vcan vcani
[ -e /sys/class/net/vcanj ] || vcan vcanj

trap 'kill $attacker' EXIT

attacker-gw & attacker=$!
attack-scenario
