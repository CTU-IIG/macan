#!/bin/bash -x

sudo modprobe peak_usb
sudo ip link set can0 up type can bitrate 125000

sudo modprobe vcan
sudo ip link add vw_can type vcan
sudo ip link set up dev vw_can
