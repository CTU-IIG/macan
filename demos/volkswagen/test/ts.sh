#!/bin/bash

source ./path.sh
clear
echo "BASH: Starting VW timeserver"
$PATH_TO_VW_BIN/macants -l 5000 -t 02 -k 01 -f 1 -c config/ts.json
