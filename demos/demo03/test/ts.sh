#!/bin/sh

source ./path.sh
clear
echo "BASH: Starting VW timeserver"
$PATH_TO_VW_BIN/macants -l 500 -t 02 -k 01 -f 1 -c $PATH_TO_VW_CONFIG/ts.json
