#include <stdio.h>
#include <omp.h>

#define NPOINTS 1600
#define MAXITER 1000

struct complex
{
    double r;
    double i;
};

void compute_serial(double *area, double *error)
{
    int numinside = 0, numoutside = 0;
    for (int i = 0; i < NPOINTS; i++)
        for (int j = 0; j < NPOINTS; j++)
        {
            // generate grid of points C in the rectangle
            // C.r in [-2  .. 0.5]
            // C.i in [0 .. 1.125 ] - will be taken also symmetric Ox
            struct complex c;
            c.r = -2.0 + 2.5 * (double)(i) / (double)(NPOINTS);
            c.i = 1.125 * (double)(j) / (double)(NPOINTS);
            struct complex z;
            z = c; // start computing series z for c
            for (int iter = 0; iter < MAXITER; iter++)
            {
                double temp = (z.r * z.r) - (z.i * z.i) + c.r;
                z.i = z.r * z.i * 2 + c.i;
                z.r = temp;
                if ((z.r * z.r + z.i * z.i) > 4.0)
                { // z diverges
                    numoutside++;
                    break;
                }
            }
        }
    numinside = NPOINTS * NPOINTS - numoutside;
    *area = 2.0 * 2.5 * 1.125 * (double)(numinside) / (double)(NPOINTS * NPOINTS);
    *error = *area / (double)NPOINTS;
}

void compute_parallel(double *area, double *error, int schedule_type, int chunk_size)
{
    int numinside = 0, numoutside = 0;

    if (schedule_type == 0) // Static scheduling
    {
        #pragma omp parallel for schedule(static, chunk_size) reduction(+:numoutside)
        for (int i = 0; i < NPOINTS; i++)
        {
            for (int j = 0; j < NPOINTS; j++)
            {
                struct complex c;
                c.r = -2.0 + 2.5 * (double)(i) / (double)(NPOINTS);
                c.i = 1.125 * (double)(j) / (double)(NPOINTS);
                struct complex z = c;

                for (int iter = 0; iter < MAXITER; iter++)
                {
                    double temp = (z.r * z.r) - (z.i * z.i) + c.r;
                    z.i = z.r * z.i * 2 + c.i;
                    z.r = temp;

                    if ((z.r * z.r + z.i * z.i) > 4.0)
                    {
                        numoutside++;
                        break;
                    }
                }
            }
        }
    }
    else if (schedule_type == 1) // Dynamic scheduling
    {
        #pragma omp parallel for schedule(dynamic, chunk_size) reduction(+:numoutside)
        for (int i = 0; i < NPOINTS; i++)
        {
            for (int j = 0; j < NPOINTS; j++)
            {
                struct complex c;
                c.r = -2.0 + 2.5 * (double)(i) / (double)(NPOINTS);
                c.i = 1.125 * (double)(j) / (double)(NPOINTS);
                struct complex z = c;

                for (int iter = 0; iter < MAXITER; iter++)
                {
                    double temp = (z.r * z.r) - (z.i * z.i) + c.r;
                    z.i = z.r * z.i * 2 + c.i;
                    z.r = temp;

                    if ((z.r * z.r + z.i * z.i) > 4.0)
                    {
                        numoutside++;
                        break;
                    }
                }
            }
        }
    }

    numinside = NPOINTS * NPOINTS - numoutside;
    *area = 2.0 * 2.5 * 1.125 * (double)(numinside) / (double)(NPOINTS * NPOINTS);
    *error = *area / (double)NPOINTS;
}

int main()
{
    double area, error;
    double start, time;

    printf("Serial version:...");
    start = omp_get_wtime();
    compute_serial(&area, &error);
    time = omp_get_wtime() - start;
    printf("Serial :  area=%f  error=%f   time=%f \n", area, error, time);

    int chunk_sizes[] = {1, 20};
    char *schedule_types[] = {"static", "dynamic"};

    for (int sched = 0; sched < 2; sched++)
    {
        for (int chunk = 0; chunk < 2; chunk++)
        {
            printf("\nParallel version - %s scheduling, chunk size = %d:\n", schedule_types[sched], chunk_sizes[chunk]);
            start = omp_get_wtime();
            compute_parallel(&area, &error, sched, chunk_sizes[chunk]);
            time = omp_get_wtime() - start;
            printf("%s scheduling (chunk size = %d): area=%f  error=%f   time=%f\n", 
                    schedule_types[sched], chunk_sizes[chunk], area, error, time);
        }
    }

    return 0;
}