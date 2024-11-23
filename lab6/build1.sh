#!/bin/bash

gcc -o omp_heat omp_heat.c -fopenmp -lm

if [ $? -eq 0 ]; then
    echo "Compilation successful"
else
    echo "Compilation failed"
    exit 1
fi

./omp_heat