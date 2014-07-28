#!/bin/bash

source ./path.sh
clear
echo "BASH: Starting CTU keyserver"
$PATH_TO_CTU_BIN/keysvr -c $PATH_TO_CTU_CONFIG/libdemo_1signal_cfg.so -k $PATH_TO_CTU_CONFIG/libdemo_1signal_keys.so
