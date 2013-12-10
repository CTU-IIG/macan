#!/bin/sh

source ./path.sh
clear
echo "BASH: Starting MACANMON"
$PATH_TO_CTU_BIN/../bin/macanmon -c $PATH_TO_CTU_CONFIG/libdemo03cfg.so 
