/****************************************************************************
 * FILE: mpi_heat2D.c
 * 
 * adaptation of example from https://hpc-tutorials.llnl.gov/mpi/examples/mpi_heat2D.c
 * 
 * DESCRIPTIONS:  
 *   HEAT2D Example - Parallelized C Version
 *   This example is based on a simplified two-dimensional heat 
 *   equation domain decomposition.  The initial temperature is computed to be 
 *   high in the middle of the domain and zero at the boundaries.  The 
 *   boundaries are held at zero throughout the simulation.  During the 
 *   time-stepping, an array containing two domains is used; these domains 
 *   alternate between old data and new data.
 *
 *   In this parallelized version, the grid is decomposed by the master
 *   process and then distributed by rows to the worker processes.  At each 
 *   time step, worker processes must exchange border data with neighbors, 
 *   because a grid point's current temperature depends upon it's previous
 *   time step value plus the values of the neighboring grid points.  Upon
 *   completion of all time steps, the worker processes return their results
 *   to the master process.
 *
 *   Two data files are produced: an initial data set and a final data set.
 * AUTHOR: Blaise Barney - adapted from D. Turner's serial C version. Converted
 *   to MPI: George L. Gusciora (1/95)
 * LAST REVISED: 06/12/13 Blaise Barney
 ****************************************************************************/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>


#define NXPROB      500                /* x dimension of problem grid */
#define NYPROB      500                /* y dimension of problem grid */
#define STEPS       300                /* number of time steps */
#define MAXWORKER   8                  /* maximum number of worker tasks */
#define MINWORKER   3                  /* minimum number of worker tasks */
#define BEGIN       1                  /* message tag */
#define LTAG        2                  /* message tag */
#define RTAG        3                  /* message tag */
#define NONE        0                  /* indicates no neighbor */
#define DONE        4                  /* message tag */
#define MASTER      0                  /* taskid of first process */

struct Parms { 
  float cx;
  float cy;
} parms = {0.1, 0.1};

void update(int start, int end, int ny, float *u1, float *u2);
void inidat(int nx, int ny, float *u);
void prtdat(int nx, int ny, float *u1, char *fnam);

int main (int argc, char *argv[])
{
    float *u[2];  // 2D grid for current and next time steps
    int taskid, numtasks, numworkers;
    int rows_per_process, extra_rows;
    int my_offset, my_rows;
    int left_neighbor, right_neighbor;
    int start, end, iz;
    
    // Arrays for scatter/gather
    int *sendcounts, *displs;
    
    MPI_Request send_request[4];  // For non-blocking communication
    MPI_Status status[4];

    // MPI Initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    
    numworkers = numtasks;  // Now including master
    
    // Allocate memory for send counts and displacements
    sendcounts = malloc(numtasks * sizeof(int));
    displs = malloc(numtasks * sizeof(int));
    
    // Calculate work distribution
    rows_per_process = NXPROB / numworkers;
    extra_rows = NXPROB % numworkers;
    
    // Prepare send counts and displacements
    int total_offset = 0;
    for (int i = 0; i < numtasks; i++) {
        if (i < extra_rows) {
            sendcounts[i] = (rows_per_process + 1) * NYPROB;
            displs[i] = total_offset;
            total_offset += sendcounts[i];
        } else {
            sendcounts[i] = rows_per_process * NYPROB;
            displs[i] = total_offset;
            total_offset += sendcounts[i];
        }
    }
    
    // Determine local chunk size and offset for this process
    if (taskid < extra_rows) {
        my_rows = rows_per_process + 1;
        my_offset = taskid * my_rows;
    } else {
        my_rows = rows_per_process;
        my_offset = taskid * rows_per_process + extra_rows;
    }
    
    // Allocate memory for grid
    u[0] = malloc(NXPROB * NYPROB * sizeof(float));
    u[1] = malloc(NXPROB * NYPROB * sizeof(float));
    
    // Determine neighbors
    left_neighbor = (taskid > 0) ? taskid - 1 : MPI_PROC_NULL;
    right_neighbor = (taskid < numworkers - 1) ? taskid + 1 : MPI_PROC_NULL;
    
    // Master initializes the entire grid and scatters it
    if (taskid == MASTER) {
        // Initialize grid
        printf("Grid size: X= %d  Y= %d  Time steps= %d\n", NXPROB, NYPROB, STEPS);
        printf("Performing heat diffusion with %d processes\n", numtasks);
        
        inidat(NXPROB, NYPROB, u[0]);
        
        // Write initial data
        prtdat(NXPROB, NYPROB, u[0], "initial.dat");
    }
    
    // Scatter initial data to all processes
    MPI_Scatterv(
        u[0],           // sendbuf
        sendcounts,     // sendcounts array
        displs,         // displacements array
        MPI_FLOAT, 
        u[0] + my_offset * NYPROB, // recvbuf
        my_rows * NYPROB, 
        MPI_FLOAT, 
        MASTER, 
        MPI_COMM_WORLD
    );
    
    // Determine computation range
    start = (my_offset == 0) ? 1 : 0;
    end = (my_offset + my_rows == NXPROB) ? my_rows - 2 : my_rows - 1;
    
    // Simulation steps
    iz = 0;
    for (int it = 1; it <= STEPS; it++) {
        // Overlap communication and computation
        
        // Non-blocking send/receive of left boundary
        if (left_neighbor != MPI_PROC_NULL) {
            MPI_Isend(u[iz] + my_offset * NYPROB, NYPROB, MPI_FLOAT, 
                      left_neighbor, 0, MPI_COMM_WORLD, &send_request[0]);
            MPI_Irecv(u[iz] + (my_offset - 1) * NYPROB, NYPROB, MPI_FLOAT, 
                      left_neighbor, 0, MPI_COMM_WORLD, &send_request[1]);
        }
        
        // Non-blocking send/receive of right boundary
        if (right_neighbor != MPI_PROC_NULL) {
            MPI_Isend(u[iz] + (my_offset + my_rows - 1) * NYPROB, NYPROB, MPI_FLOAT, 
                      right_neighbor, 0, MPI_COMM_WORLD, &send_request[2]);
            MPI_Irecv(u[iz] + (my_offset + my_rows) * NYPROB, NYPROB, MPI_FLOAT, 
                      right_neighbor, 0, MPI_COMM_WORLD, &send_request[3]);
        }
        
        // Local computation
        update(start, end, NYPROB, u[iz] + my_offset * NYPROB, 
               u[1-iz] + my_offset * NYPROB);
        
        // Wait for boundary exchange to complete
        if (left_neighbor != MPI_PROC_NULL) {
            MPI_Waitall(2, &send_request[0], &status[0]);
        }
        if (right_neighbor != MPI_PROC_NULL) {
            MPI_Waitall(2, &send_request[2], &status[2]);
        }
        
        // Swap current and next time step grids
        iz = 1 - iz;
    }
    
    // Gather results back to master
    if (taskid == MASTER) {
        MPI_Gatherv(
            u[iz] + my_offset * NYPROB, 
            my_rows * NYPROB, 
            MPI_FLOAT,
            u[0], 
            sendcounts, 
            displs, 
            MPI_FLOAT, 
            MASTER, 
            MPI_COMM_WORLD
        );
        
        // Write final data
        printf("Writing final.dat file...\n");
        prtdat(NXPROB, NYPROB, u[0], "final.dat");
    } else {
        // Other processes send their data back
        MPI_Gatherv(
            u[iz] + my_offset * NYPROB, 
            my_rows * NYPROB, 
            MPI_FLOAT,
            NULL, 
            NULL, 
            NULL, 
            MPI_FLOAT, 
            MASTER, 
            MPI_COMM_WORLD
        );
    }
    
    // Free allocated memory
    free(u[0]);
    free(u[1]);
    free(sendcounts);
    free(displs);
    
    // Finalize MPI
    MPI_Finalize();
    return 0;
}


/**************************************************************************
 *  subroutine update
 ****************************************************************************/
void update(int start, int end, int ny, float *u1, float *u2)
{
   int ix, iy;
   for (ix = start; ix <= end; ix++) 
      for (iy = 1; iy <= ny-2; iy++) 
         *(u2+ix*ny+iy) = *(u1+ix*ny+iy)  + 
                          parms.cx * (*(u1+(ix+1)*ny+iy) +
                          *(u1+(ix-1)*ny+iy) - 
                          2.0 * *(u1+ix*ny+iy)) +
                          parms.cy * (*(u1+ix*ny+iy+1) +
                         *(u1+ix*ny+iy-1) - 
                          2.0 * *(u1+ix*ny+iy));
}

/*****************************************************************************
 *  subroutine inidat
 *****************************************************************************/
void inidat(int nx, int ny, float *u) {
int ix, iy;
// apply heat in the middle, zero at margins
for (ix = 0; ix <= nx-1; ix++) 
  for (iy = 0; iy <= ny-1; iy++)
     *(u+ix*ny+iy) = (float)(ix * (nx - ix - 1) * iy * (ny - iy - 1));

}

/**************************************************************************
 * subroutine prtdat
 **************************************************************************/
void prtdat(int nx, int ny, float *u1, char *fnam) {
int ix, iy;
FILE *fp;

fp = fopen(fnam, "w");
for (iy = ny-1; iy >= 0; iy--) {
  for (ix = 0; ix <= nx-1; ix++) {
    fprintf(fp, "%8.1f", *(u1+ix*ny+iy));
    if (ix != nx-1) 
      fprintf(fp, " ");
    else
      fprintf(fp, "\n");
    }
  }
fclose(fp);
}