#!/bin/bash

#Helper script to save user from having to write out all the arguments.
#Runs klee in the current directory on the first argument.

klee --only-output-states-covering-new --optimize --search=dfs \
     --max-memory=0 --check-div-zero --check-overshift $1
