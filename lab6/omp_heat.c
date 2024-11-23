/**
 * Heat 2D 
 * A rectangular plane of material has heat applied to the center of the upper edge.
 * The rectangular plane is represented as a grid of points.
 * The Laplace equation dictates how the heat will transfer from grid point to grid point over time.
 * The program simulates the diffusion of temperature in all points over time.
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <omp.h>

#define N 4000 /* grid points */
#define MAXITER 100
#define NTHREADS 8

// #define DEBUG

double *grid; // the grid and next generation grid
double *new_grid;

double *groundtruth; // saved result for comparison

void init_temperatures(void)
{
    grid = (double *)malloc(N * N * sizeof(double));
    if (!grid)
    {
        printf("Memory allocation error for grid\n");
        exit(1);
    }
    new_grid = (double *)malloc(N * N * sizeof(double));
    if (!new_grid)
    {
        printf("Memory allocation error for new grid\n");
        exit(1);
    }

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
        {
            grid[i * N + j] = 20;
        }
    /* init boundary conditions:
    apply heat on middle part of upper boundary */
    for (int j = N / 3; j < N * 2 / 3; j++)
    {
        grid[0 * N + j] = 100;
    }
}

void print_grid(void)
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            printf("%5.1lf ", grid[i * N + j]);
        }
        printf("\n");
    }
}

int equal_groundtruth(void)
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
        {
            if (grid[i * N + j] != groundtruth[i * N + j])
                return 0;
        }
    return 1;
}

void save_groundtruth(void)
{
    groundtruth = (double *)malloc(N * N * sizeof(double));
    if (!groundtruth)
    {
        printf("Memory allocation error for groundtruth result\n");
        exit(1);
    }

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
        {
            groundtruth[i * N + j] = grid[i * N + j];
        }
}

void swap_ptr(double **p1, double **p2)
{
    double *tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
}

void serial_temp()
{
    int i, j, time;

    for (time = 0; time < MAXITER; time++)
    {
#ifdef DEBUG
        printf("\nIteration %d \n", time);
        print_grid();
#endif
        for (i = 1; i < N - 1; i++) // iterate grid but skip boundary
            for (j = 1; j < N - 1; j++)
            {
                new_grid[i * N + j] = (grid[(i + 1) * N + j] +
                                       grid[(i - 1) * N + j] +
                                       grid[i * N + j + 1] +
                                       grid[i * N + j - 1]) *
                                      0.25;
            }

        // Make new grid to current grid for the next generation
        swap_ptr(&grid, &new_grid);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
    I experimented with different scheduling strategies and chunk sizes to see how they affect the performance:
        default -> Parallel Time 3.698069  Speedup 2.924917
        schedule(dynamic, 10) -> Parallel Time 1.812749  Speedup 4.396572
        schedule(static, 20) -> Parallel Time 1.701645  Speedup 3.947440
        schedule(guided, 8) -> Parallel Time 1.705210  Speedup 3.936668
        schedule(dynamic, 50) -> Parallel Time 1.735867  Speedup 3.620426
    
    Best performance: schedule(dynamic, 10) 
*/
void parallel_temp()
{
    for (int time = 0; time < MAXITER; time++)
    {
        #pragma omp parallel for num_threads(NTHREADS) default(none) shared(grid, new_grid) //schedule(dynamic, 50)
        for (int i = 1; i < N - 1; i++) // iterate grid but skip boundary
            for (int j = 1; j < N - 1; j++)
            {
                new_grid[i * N + j] = (grid[(i + 1) * N + j] +
                                       grid[(i - 1) * N + j] +
                                       grid[i * N + j + 1] +
                                       grid[i * N + j - 1]) *
                                      0.25;
            }
        // Make new grid to current grid for the next generation
        swap_ptr(&grid, &new_grid);
    }
}

/*
    O abordare diferita pt paralelizarea acestei probleme ari fi sa folosim o
    regiune paralela si sa partitionam totul manual in loc sa ne bazam pe
    OpenMP parallel for. Astfel, fiecare thread va lucra pe un subset de linii 
    si le va actualiza independent.
*/

void parallel_temp_manual_partitioning()
{
    #pragma omp parallel num_threads(NTHREADS) 
    {
        int id = omp_get_thread_num();
        int nthrds = omp_get_num_threads();
        int rows_per_thread = N / nthrds;
        
        int start_row = id * rows_per_thread;
        int end_row;

        if(id==nthrds-1)
            end_row = N-1;
        else end_row=start_row+rows_per_thread;

        
        for (int time = 0; time < MAXITER; time++)
        {
            for (int i = start_row; i < end_row; i++)
                for (int j = 1; j < N - 1; j++)
                {
                    if (i > 0 && i < N - 1) // Skip boundary
                    {
                        new_grid[i * N + j] = (grid[(i + 1) * N + j] +
                                               grid[(i - 1) * N + j] +
                                               grid[i * N + j + 1] +
                                               grid[i * N + j - 1]) * 0.25;
                    }
                }

            #pragma omp barrier
            if (id == 0) // Only one thread swaps the pointers
                swap_ptr(&grid, &new_grid);
            #pragma omp barrier
        }
    }
}


int main(int argc, char *argv[])
{

    double start, end, serial, parallel;

    printf("Initialize grid size N=%d\n", N);
    init_temperatures();

    printf("Start Serial with MAXITER=%d\n", MAXITER);
    start = omp_get_wtime();
    serial_temp();
    end = omp_get_wtime();
    serial = end - start;
    printf("Serial Time %lf \n", serial);
    save_groundtruth(); // keep values from serial result as ground truth for later comparison

    printf("Initialize grid size N=%d\n", N);
    init_temperatures(); // init again the same grid for parallel version

    printf("Start Paralel with NTHREADS=%d\n", NTHREADS);
    start = omp_get_wtime();
    parallel_temp();
    end = omp_get_wtime();
    parallel = end - start;
    printf("Parallel Time %lf  Speedup %lf \n", parallel, serial / parallel);


    /// for testing part2
    printf("Initialize grid size N=%d\n", N);
    init_temperatures(); // init again the same grid for the new parallel version
    printf("Start New Paralel with NTHREADS=%d\n", NTHREADS);
    start = omp_get_wtime();
    parallel_temp_manual_partitioning();
    end = omp_get_wtime();
    parallel = end - start;
    printf("New Parallel Time %lf  Speedup %lf \n", parallel, serial / parallel);



    if (!equal_groundtruth())
        printf("!!! Parallel version produces a different result! \n");
    else
        printf("Parallel version produced the same result \n");
}