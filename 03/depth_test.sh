#!/bin/bash

for d in 4 5 6 7 8 9; do
    g++ -DDEPTH=$d -DTHREADS=4 -fopenmp -Ofast main.cpp -o main
    ./main.exe test/graf_30_20.txt 15
done
read