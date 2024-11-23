#!/bin/bash

gcc -Wall -o omp_mergesort_skel omp_mergesort_skel.c -fopenmp 

if [ $? -eq 0 ]; then
    echo "Compilation successful"
    ./omp_mergesort_skel 
else
    echo "Compilation failed"
fi