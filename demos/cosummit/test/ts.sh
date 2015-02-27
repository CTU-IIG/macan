#!/bin/bash

source ./path.sh
clear
echo "BASH: Starting CTU timeserver"
$PATH_TO_CTU_BIN/timesvr -c $PATH_TO_CTU_CONFIG/libdemo_cosummit_cfg.so -k $PATH_TO_CTU_CONFIG/libdemo_cosummit_tskey.so
