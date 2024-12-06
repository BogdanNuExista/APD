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

int main(int argc, char *argv[])
{
    float  u[2][NXPROB][NYPROB];        /* array for grid */
    int    taskid, numtasks, rows, offset;
    int    iz, it;
    MPI_Status status;

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

    // Calculate rows per process
    rows = NXPROB / numtasks;
    offset = taskid * rows;

    // Initialize grid and print initial state only on master process
    if (taskid == MASTER) {
        printf("Grid size: X= %d  Y= %d  Time steps= %d\n", NXPROB, NYPROB, STEPS);
        printf("Initializing grid and initial.dat file...\n");
        inidat(NXPROB, NYPROB, &u[0][0][0]);
        prtdat(NXPROB, NYPROB, &u[0][0][0], "initial.dat");
    }

    // Scatter initial data to all processes
    MPI_Scatter(&u[0][0][0], rows*NYPROB, MPI_FLOAT, 
                &u[0][offset][0], rows*NYPROB, MPI_FLOAT, 
                MASTER, MPI_COMM_WORLD);

    // Determine local start and end for each process
    int start = (taskid == 0) ? 1 : offset;
    int end = (taskid == numtasks-1) ? NXPROB-2 : offset+rows-1;

    // Compute local rows
    iz = 0;
    for (it = 1; it <= STEPS; it++)
    {
        // Non-blocking communication
        MPI_Request left_recv_req, left_send_req;
        MPI_Request right_recv_req, right_send_req;

        // Left neighbor communication
        if (taskid > 0) {
            MPI_Irecv(&u[iz][offset-1][0], NYPROB, MPI_FLOAT, taskid-1, 0, MPI_COMM_WORLD, &left_recv_req);
            MPI_Isend(&u[iz][offset][0], NYPROB, MPI_FLOAT, taskid-1, 0, MPI_COMM_WORLD, &left_send_req);
        }

        // Right neighbor communication
        if (taskid < numtasks-1) {
            MPI_Irecv(&u[iz][offset+rows][0], NYPROB, MPI_FLOAT, taskid+1, 0, MPI_COMM_WORLD, &right_recv_req);
            MPI_Isend(&u[iz][offset+rows-1][0], NYPROB, MPI_FLOAT, taskid+1, 0, MPI_COMM_WORLD, &right_send_req);
        }

        // Local computation (independent of communication)
        update(start, end, NYPROB, &u[iz][0][0], &u[1-iz][0][0]);

        // Wait for communication to complete
        if (taskid > 0)
            MPI_Wait(&left_recv_req, &status);
        if (taskid < numtasks-1)
            MPI_Wait(&right_recv_req, &status);

        // Swap arrays
        iz = 1 - iz;
    }

    // Gather final results from all processes
    MPI_Gather(&u[iz][offset][0], rows*NYPROB, MPI_FLOAT, 
               &u[0][0][0], rows*NYPROB, MPI_FLOAT, 
               MASTER, MPI_COMM_WORLD);

    // Write final data on master process
    if (taskid == MASTER) {
        printf("Writing final.dat file...\n");
        prtdat(NXPROB, NYPROB, &u[0][0][0], "final.dat");
    }

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