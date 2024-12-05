#!/bin/bash

mpicc -g -Wall -o mpi_heat2D mpi_heat2D.c
mpiexec -n 4 ./mpi_heat2D 
rm -f mpi_heat2D