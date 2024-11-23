#!/bin/bash

gcc -Wall -o p1 abc.c
if [ $? -ne 0 ]; then
    echo "Compilation for abc.c failed"
    exit 1
fi

gcc -Wall -o p2 wrong_counter.c
if [ $? -ne 0 ]; then
    echo "Compilation for wrong_counter.c failed"
    exit 1
fi

gcc -Wall -o p3 bounded_buff_condvar.c
if [ $? -ne 0 ]; then
    echo "Compilation for bounded_buff_condvar.c failed"
    exit 1
fi

echo "exercice 1 output:"
./p1
echo ""

echo "exercice 2 output:"
./p2
echo ""

echo "exercice 3 output:"
./p3
echo ""