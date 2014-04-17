#!/bin/bash

source ./path.sh
clear
echo "BASH: Starting VW keyserver"
$PATH_TO_VW_BIN/macanks -l 5000 -f config/ks.json -k 01
