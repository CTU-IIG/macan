#!/bin/bash

HERE=$(pwd -P)
C=${HERE%/demos/cosummit/test}/build/linux/_compiled
export MACAN_DUMP=1 # Serve as macanmon too
exec $C/bin/macan_ksts -c $C/lib/libdemo_cosummit_cfg.so -k $C/lib/libdemo_cosummit_keys.so
