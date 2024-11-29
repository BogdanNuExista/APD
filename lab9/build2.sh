#!/bin/bash

mpicc -g -Wall -o mpi_trap2_barrier mpi_trap2_barrier.c
mpiexec -n 4 ./mpi_trap2_barrier
rm mpi_trap2_barrier