/* File:       
 *    mpi_hello.c
 *
 * Purpose:    
 *    A "hello,world" program that uses MPI
 *
 * Compile:    
 *    mpicc -g -Wall -std=C99 -o mpi_hello mpi_hello.c
 * Usage:        
 *    mpiexec -n<number of processes> ./mpi_hello
 *
 * Input:      
 *    None
 * Output:     
 *    A greeting from each process
 *
 * Algorithm:  
 *    Each process sends a message to process 0, which prints 
 *    the messages it has received, as well as its own message.
 *
 * IPP2  Section 3.1 (pp. 90 and ff.)
 */
#include <stdio.h>
#include <string.h>  /* For strlen             */
#include "mpi.h"    /* For MPI functions, etc */ 

#define MAX_STRING   100

int main(void) {
    int my_rank;
    int comm_sz;
    int numbers[2] = {10, 20};  // Example numbers to send
    int sum;
    MPI_Status status;

    /* Start up MPI */
    MPI_Init(NULL, NULL);
    
    /* Get the number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    
    /* Get my rank among all the processes */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (comm_sz != 2) {
        printf("This program requires exactly 2 processes\n");
        MPI_Finalize();
        return 1;
    }

    if (my_rank == 0) {
        // Process 0 sends two numbers to process 1
        MPI_Send(numbers, 2, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("Process 0 sent numbers %d and %d to process 1\n", numbers[0], numbers[1]);
        
        // Process 0 receives the sum from process 1
        MPI_Recv(&sum, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
        printf("Sum received from process 1: %d\n", sum);
    } 
    else if (my_rank == 1) {
        int received_numbers[2];
        
        // Process 1 receives the two numbers
        MPI_Recv(received_numbers, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        
        // Calculate sum
        sum = received_numbers[0] + received_numbers[1];
        printf("Process 1 calculated sum: %d\n", sum);
        
        // Send sum back to process 0
        MPI_Send(&sum, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
} /* main */