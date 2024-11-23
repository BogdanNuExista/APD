#!/bin/bash

# Compile the program
gcc -fopenmp main.c game.c -o tictactoe

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful! Running the program..."
    ./tictactoe
else
    echo "Compilation failed!"
fi