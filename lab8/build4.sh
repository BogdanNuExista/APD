#!/bin/bash

mpicc -g -Wall -std=c99 -o mpi_ring_deadlock2 mpi_ring_deadlock2.c

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    mpirun -np 4 ./mpi_ring_deadlock2
else
    echo "Compilation failed!"
fi