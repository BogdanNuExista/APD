#!/bin/bash

mpicc -g -Wall -o mpi_trap2 mpi_trap2.c -lm
mpiexec -n 4 ./mpi_trap2
rm mpi_trap2