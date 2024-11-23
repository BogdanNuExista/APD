#!/bin/bash

# for exercice 3

gcc -o performance_pth_mat_vect performance_pth_mat_vect.c -lpthread -lm

if [ $? -eq 0 ]; then
    echo "Compilation performance_pth_mat_vect.c success"
else
    echo "Compilation performance_pth_mat_vect.c failed"
fi

# for m 8000 n 8000

echo "Performance for m = 8000, n = 8000" > performance2.txt
./performance_pth_mat_vect 2 << EOF >> performance2.txt
8000
8000
EOF
echo "" >> performance2.txt

./performance_pth_mat_vect 4 << EOF >> performance2.txt
8000
8000
EOF
echo "" >> performance2.txt

./performance_pth_mat_vect 8 << EOF >> performance2.txt
8000
8000
EOF
echo "" >> performance2.txt

#For m = 8, n = 8000000

echo "Performance for m = 8, n = 8000000" >> performance2.txt
./performance_pth_mat_vect 2 << EOF >> performance2.txt
8
8000000
EOF
echo "" >> performance2.txt

./performance_pth_mat_vect 4 << EOF >> performance2.txt
8
8000000
EOF
echo "" >> performance2.txt

./performance_pth_mat_vect 8 << EOF >> performance2.txt
8
8000000
EOF
echo "" >> performance2.txt

#For m = 8000000, n = 8

echo "Performance for m = 8000000, n = 8" >> performance2.txt
./performance_pth_mat_vect 2 << EOF >> performance2.txt
8000000
8
EOF
echo "" >> performance2.txt

./performance_pth_mat_vect 4 << EOF >> performance2.txt
8000000
8
EOF
echo "" >> performance2.txt

./performance_pth_mat_vect 8 << EOF >> performance2.txt
8000000
8
EOF
echo "" >> performance2.txt

