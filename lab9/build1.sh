#!/bin/bash

mpicc -g -Wall -o mpi_trap2_bcast_reduce mpi_trap2_bcast_reduce.c
mpiexec -n 4 ./mpi_trap2_bcast_reduce
rm mpi_trap2_bcast_reduce