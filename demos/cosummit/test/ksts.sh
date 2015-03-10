#!/bin/bash

source ./path.sh
export MACAN_DUMP=1 # Serve as macanmon too
exec $PATH_TO_CTU_BIN/macan_ksts -c $PATH_TO_CTU_CONFIG/libdemo_cosummit_cfg.so -k $PATH_TO_CTU_CONFIG/libdemo_cosummit_keys.so
