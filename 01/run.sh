#!/bin/bash

g++ -Ofast main.cpp -o main

as=(5 5 5 5 7 10 10 10 10 15 15 15 20)
i=0
for file in test/*; do
    ./main.exe $file ${as[i]}
    ((i=i+1))

done
read