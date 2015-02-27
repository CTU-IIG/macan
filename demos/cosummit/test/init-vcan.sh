#!/bin/bash -x

sudo modprobe vcan

sudo ip link add can0 type vcan
sudo ip link set up dev can0

sudo ip link add vw_can type vcan
sudo ip link set up dev vw_can
