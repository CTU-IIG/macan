#!/bin/bash

source ./path.sh
clear
echo "BASH: Starting CTU timeserver"
$PATH_TO_CTU_BIN/timesvr -c $PATH_TO_CTU_CONFIG/libdemo_4signals_cfg.so -k $PATH_TO_CTU_CONFIG/libdemo_4signals_tskey.so
