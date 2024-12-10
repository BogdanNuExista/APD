#!/bin/bash

# Compile the program
echo "Compiling dispatcher program..."
mpicc -o dispatcher main.c -lm

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    
    # Check if a command file was provided as argument
    if [ "$1" != "" ]; then
        echo "Starting dispatcher with 4 processes..."
        
        # Capture the time output
        { time mpirun -np 4 ./dispatcher "$1"; } 2> time_output.txt
        
        # Echo the time at the end
        echo "Execution time:"
        cat time_output.txt
        
        # Clean up
        rm -f dispatcher time_output.txt
    else
        echo "Usage: ./run_dispatcher.sh <command_file>"
        echo "Please provide a command file as argument"
    fi
else
    echo "Compilation failed!"
fi