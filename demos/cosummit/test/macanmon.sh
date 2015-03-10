#!/bin/bash

source ./path.sh
clear
echo "BASH: Starting MACANMON"
exec $PATH_TO_CTU_BIN/../bin/macanmon -c $PATH_TO_CTU_CONFIG/libdemo_cosummit_cfg.so 
