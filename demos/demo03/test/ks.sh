#!/bin/sh

source ./path.sh
clear
echo "BASH: Starting VW keyserver"
$PATH_TO_VW_BIN/macanks -l 500 -f $PATH_TO_VW_CONFIG/ks.json -k 01
