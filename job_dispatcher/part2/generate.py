import random

matrix_size = 150
matrix = [[random.randint(0, 100) for _ in range(matrix_size)] for _ in range(matrix_size)]

with open('/home/bogdan/labs/apd/job_dispatcher/part2/matrix2.txt', 'w') as f:
    for row in matrix:
        f.write(' '.join(map(str, row)) + '\n')