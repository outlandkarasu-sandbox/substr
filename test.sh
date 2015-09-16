#!/bin/sh

for d in `ls ./dataset`
do
  echo ""
  echo -------- ${d} --------
  echo ""
  time sh -c "cat ./dataset/${d} | ./substr"
done
