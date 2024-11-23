#!/bin/bash

gcc -o omp_countsort_skel omp_countsort_skel.c -fopenmp -lm #-DDEBUG

if [ $? -eq 0 ]; then
    echo "Compilation successful"
    ./omp_countsort_skel 4 50000 #> output.txt  # 4 threads, 50000 elements
else
    echo "Compilation failed"
    exit 1
fi