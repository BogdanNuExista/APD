#!/bin/bash

# ONLY FOR EXERCICE 2

gcc -Wall -o deadlock deadlock.c 

if [ $? -eq 0 ]; then
    echo "deadlock.c compiled successfully"
else
    echo "deadlock.c failed to compile"
fi

gcc -Wall -o performancecount1 performance_count1.c

if [ $? -eq 0 ]; then
    echo "performancecount1.c compiled successfully"
else
    echo "performancecount1.c failed to compile"
fi

gcc -Wall -o performancecount2 performance_count2.c

if [ $? -eq 0 ]; then
    echo "performancecount2.c compiled successfully"
else
    echo "performancecount2.c failed to compile"
fi

gcc -Wall -o performancecount3 performance_count3.c

if [ $? -eq 0 ]; then
    echo "performancecount3.c compiled successfully"
else
    echo "performancecount3.c failed to compile"
fi

gcc -Wall -o performancecount4 performance_count4.c

if [ $? -eq 0 ]; then
    echo "performancecount4.c compiled successfully"
else
    echo "performancecount4.c failed to compile"
fi

echo "Output of performancecount1:" > performance.txt 
./performancecount1 >> performance.txt

echo "" >> performance.txt
echo "Output of performancecount2:" >> performance.txt
./performancecount2 >> performance.txt

echo "" >> performance.txt
echo "Output of performancecount3:" >> performance.txt
./performancecount3 >> performance.txt

echo "" >> performance.txt
echo "Output of performancecount4:" >> performance.txt
./performancecount4 >> performance.txt

