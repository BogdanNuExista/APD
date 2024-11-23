#!/bin/bash

mpicc -g -Wall -std=c99 -o mpi_first mpi_first.c

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    mpirun -np 4 ./mpi_first
else
    echo "Compilation failed!"
fi