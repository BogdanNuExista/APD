#!/bin/bash


# Compile the unified program
echo "Compiling dispatcher program..."
mpicc -o dispatcher main.c -lm

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    
    # Check if a command file was provided as argument
    if [ "$1" != "" ]; then
        # Run the program with 4 processes (1 main server + 3 workers)
        echo "Starting dispatcher with 4 processes..."
        mpirun -np 4 ./dispatcher "$1"
        rm -f dispatcher
    else
        echo "Usage: ./build.sh <command_file>"
        echo "Please provide a command file as argument"
    fi
else
    echo "Compilation failed!"
fi