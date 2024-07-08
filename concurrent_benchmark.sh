#!/bin/bash

n_clients=$(($1 + 0))
echo "Starting $n_clients clients"
mkdir "concurrent_$n_clients"
for i in $(seq 1 $n_clients) ;
do
    ( ./client.out "0.0.0.0" "5000" $n_clients "10" "10000" > "concurrent_${n_clients}/concurrent_${n_clients}_${i}.benchmark" & )
done