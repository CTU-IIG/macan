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
    echo "req_auth:1:1"
    cat
) | $PATH_TO_VW_BIN/macangw -o 3 -t 5000 -b can0 -e vw_can -k can0 -c config/ecuvw.json -s config/signals_ecuvw.json
