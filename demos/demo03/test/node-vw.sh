#!/bin/sh

source ./path.sh
clear
echo "BASH: Starting VW node"
$PATH_TO_VW_BIN/macangw -o 3 -t 500 -b can0 -e vw_can -k can0 -c $PATH_TO_VW_CONFIG/ecuvw.json -s $PATH_TO_VW_CONFIG/signals_ecuvw.json
