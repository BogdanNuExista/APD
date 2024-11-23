#!/bin/bash

gcc -fopenmp matrix_mult.c -o matrix_mult -lm

if [ $? -eq 0 ]; then
    echo "Compilation successful" 
    ./matrix_mult #> output.txt
else
    echo "Compilation failed"
fi