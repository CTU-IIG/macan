#!/bin/bash

#quick n dirty script to clear KLEE's result directory from incomplete results
#Run it in directory to clear.


FILES="*early"
for f in $FILES
do
  a=`echo $f | cut -d"." -f1`
  rm "$a.pc" "$a.ktest" "$a.early"
done
