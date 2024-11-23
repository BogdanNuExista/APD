gcc -Wall -I../include -o p main.c ../src/utils.c ../src/profiling.c

if [ $? -eq 0 ]; then
    echo "Build successful"
    ./p
else
    echo "Build failed"
fi