#!/bin/bash

gcc -Wall -o omp_btree_skel omp_btree_skel.c -fopenmp

if [ $? -eq 0 ]; then
    echo "Compilation successful"
    ./omp_btree_skel
else
    echo "Compilation failed"
fi