#!/bin/sh

for n in n1000 n10000 n100000 n1000000; do
  echo ${n}
  time (cat ${n}.txt | ./substr)
done

