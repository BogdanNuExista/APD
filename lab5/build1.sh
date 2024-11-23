#!/bin/bash

gcc -Wall -fopenmp -o p skel_primes.c

if [ $? -eq 0 ]; then
    echo "Compilation successful"
else
    echo "Compilation failed"
    exit 1
fi

./p