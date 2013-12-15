#!/bin/bash -x

sudo modprobe peak_usb
sudo ip link set can0 up type can bitrate 125000
