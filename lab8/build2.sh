#!/bin/bash

mpicc -g -Wall -std=c99 -o mpi_hello mpi_hello.c

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    mpirun -np 2 ./mpi_hello
else
    echo "Compilation failed!"
fi