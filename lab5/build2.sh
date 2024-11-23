#!/bin/bash

gcc -Wall -fopenmp -lm -o p skel_mandelbrot.c

if [ $? -eq 0 ]; then
    echo "Compilation successful"
else
    echo "Compilation failed"
    exit 1
fi

./p