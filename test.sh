#!/bin/sh

for d in `ls ./dataset`
do
  echo -------- ${d} --------
  time sh -c "cat ./dataset/${d} | ./substr"
done
