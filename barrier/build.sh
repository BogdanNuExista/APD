#!/bin/bash

make clean

make all

echo "Running the barrier program"
./jones_family

echo ""
echo "Running the benchmark program"
./benchmark 4 1000 5
