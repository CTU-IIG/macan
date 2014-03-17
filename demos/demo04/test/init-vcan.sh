#!/bin/bash -x

sudo modprobe vcan

sudo ip link add can0 type vcan
sudo ip link set up dev can0
