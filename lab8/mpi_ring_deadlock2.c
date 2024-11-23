/**
 * file: mpi_ring_deadlock.c
 * demonstrates the danger of deadlock
 **/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int numtasks, rank, next, prev, a, b, tag = 1;
    MPI_Status status;
    MPI_Request request;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    prev = rank - 1;
    next = rank + 1;
    if (rank == 0)
        prev = numtasks - 1;
    if (rank == (numtasks - 1))
        next = 0;

    a = rank;

    // Start non-blocking receive
    MPI_Irecv(&b, 1, MPI_INT, prev, tag, MPI_COMM_WORLD, &request);
    
    // Send data
    MPI_Send(&a, 1, MPI_INT, next, tag, MPI_COMM_WORLD);
    printf("Task %d sent %d to task %d\n", rank, a, next);

    // Wait for receive to complete
    MPI_Wait(&request, &status);
    printf("Task %d received %d from task %d\n", rank, b, prev);

    
    MPI_Finalize();
}