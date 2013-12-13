#!/bin/bash

source ./path.sh
clear
echo "BASH: Starting VW node"

(
    echo "key:2"
    sleep 1
    echo "key:4"
    sleep 1
    echo "time"
    sleep 1
    echo "req_auth:2:1"
) | $PATH_TO_VW_BIN/macangw -o 3 -t 500 -b can0 -e vw_can -k can0 -c $PATH_TO_VW_CONFIG/ecuvw.json -s $PATH_TO_VW_CONFIG/signals_ecuvw.json
