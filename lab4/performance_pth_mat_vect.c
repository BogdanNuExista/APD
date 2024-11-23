#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAXRANGE 5000

int thread_count;
int m, n;  // size of matrix
double *A;  // matrix to be multiplied
double *x;  // vector to be multiplied
double *y;  // result vector for serial
double *y_serial; //result vector for parallel

void Usage(char *prog_name);
void Generate_matrix(char *prompt, double A[], int m, int n);
void Generate_vector(char *prompt, double x[], int n);
void Print_matrix(char *title, double A[], int m, int n);
void Print_vector(char *title, double y[], double m);
int Equal_vectors(double y[], double z[], double m);

/* Serial algo */
void Mat_vect_mult_serial(); // computes y_serial = A * x

/* Parallel algo */
void *Pth_mat_vect(void *rank);
void Mat_vect_mult_parallel(); // computes y = A * x

/*------------------------------------------------------------------*/
int main(int argc, char *argv[])
{

   if (argc != 2)
      Usage(argv[0]);
   thread_count = atoi(argv[1]);

   printf("Enter m and n\n");
   scanf("%d%d", &m, &n);

   if (m % thread_count != 0)
   {
      printf("m is not divisible by thread_count!\n");
      exit(1);
   }

printf("Total number of elements m*n= %d \n",m*n);

   A = malloc(m * n * sizeof(double));
   x = malloc(n * sizeof(double));
   y_serial = malloc(m * sizeof(double));
   y = malloc(m * sizeof(double));

   if ((A==NULL)||(x==NULL)||(y_serial==NULL)||(y==NULL)) {
    printf("Memory allocation error !\n");
    exit(1);
   }

   Generate_matrix("Generate the matrix", A, m, n);
#ifdef DEBUG
   Print_matrix("Matrix is", A, m, n);
#endif

   Generate_vector("Generate the vector", x, n);
#ifdef DEBUG
   Print_vector("Vector is", x, n);
#endif

   struct timespec start, finish;
   double elapsed_serial, elapsed_parallel;
   printf("Serial: ");
   clock_gettime(CLOCK_MONOTONIC, &start); // measure wall clock time!

   Mat_vect_mult_serial();

   clock_gettime(CLOCK_MONOTONIC, &finish);

   elapsed_serial = (finish.tv_sec - start.tv_sec);
   elapsed_serial += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

   printf("time =%lf \n", elapsed_serial);
#ifdef DEBUG
   Print_vector("Serial Result", y_serial, m);
#endif

   printf("Parallel: ");

   clock_gettime(CLOCK_MONOTONIC, &start); // measure wall clock time!

   Mat_vect_mult_parallel();

   clock_gettime(CLOCK_MONOTONIC, &finish);

   elapsed_parallel = (finish.tv_sec - start.tv_sec);
   elapsed_parallel += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

   printf("time =%lf \n", elapsed_parallel);

#ifdef DEBUG
   Print_vector("Paralell Result", y, m);
#endif

   if (!Equal_vectors(y, y_serial, m))
      printf("Error! Serial and Parallel result vectors not equal! \n");

   printf("Number of parallel threads was %d\n", thread_count);
   printf("Measured Speedup=%f\n ", elapsed_serial / elapsed_parallel);

   free(A);
   free(x);
   free(y);
   free(y_serial);

   return 0;
} /* main */

void Usage(char *prog_name)
{
   fprintf(stderr, "usage: %s <thread_count>\n", prog_name);
   exit(0);
} /* Usage */

void Generate_matrix(char *prompt, double A[], int m, int n)
{
   int i, j;

   srand(time(NULL));
   printf("%s\n", prompt);
   for (i = 0; i < m; i++)
      for (j = 0; j < n; j++)
         // scanf("%lf", &A[i*n+j]);
         A[i * n + j] = rand() % MAXRANGE;
} /* Generate_matrix */

void Generate_vector(char *prompt, double x[], int n)
{
   int i;

   printf("%s\n", prompt);
   for (i = 0; i < n; i++)
      // scanf("%lf", &x[i]);
      x[i] = rand() % MAXRANGE;
} /* Generate_vector */

int Equal_vectors(double y[], double z[], double m)
{
   int i;
   for (i = 0; i < m; i++)
      if (y[i] != z[i])
         return 0;
   return 1;
}

void Mat_vect_mult_serial()
{
   int i, j;

   for (i = 0; i < m; i++)
   {
      y_serial[i] = 0.0;
      for (j = 0; j < n; j++)
         y_serial[i] += A[i * n + j] * x[j];
   }
} /* Mat_vect_mult_serial */

void *Pth_mat_vect(void *rank)
{
   int my_rank = *(int *)rank;
   int i, j;
   int local_m = m / thread_count;
   int my_first_row = my_rank * local_m;
   int my_last_row = (my_rank + 1) * local_m - 1;

   for (i = my_first_row; i <= my_last_row; i++)
   {
      y[i] = 0.0;
      for (j = 0; j < n; j++)
         y[i] += A[i * n + j] * x[j];
   }

   return NULL;
} /* Pth_mat_vect */

///////////////////////////////////////////////////////////////////////////////////

/*
   At cand m este mic si n este mare, putem folosi block processing
   - pricesam vectorul x in blocuri de dimensiune BLOCK_SIZE
   - ajuta sa pastram datele in cache
   - reduce chache miss-uri cand accesam vectorul x
*/
#define BLOCK_SIZE 256
void *Pth_mat_vect2(void *rank) 
{
    int my_rank = *(int *)rank;
    int local_m = m / thread_count;
    int my_first_row = my_rank * local_m;
    int my_last_row = (my_rank + 1) * local_m - 1;
    
    // Process matrix in blocks
    for (int i = my_first_row; i <= my_last_row; i++) 
    {
        double temp = 0.0;  
        
        for (int j = 0; j < n; j += BLOCK_SIZE) 
        {
            // Calculate the end of current block
            int block_end = j + BLOCK_SIZE < n ? j + BLOCK_SIZE : n;
            
            // Process current block
            double block_sum = 0.0;
            for (int k = j; k < block_end; k++) 
            {
                block_sum += A[i * n + k] * x[k];
            }
            temp += block_sum;
        }
        y[i] = temp;
    }
    
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////////

void Mat_vect_mult_parallel()
{
   int thread;
   pthread_t *thread_handles;
   thread_handles = malloc(thread_count * sizeof(pthread_t));
   int *tid;
   tid = malloc(thread_count * sizeof(int));

   for (thread = 0; thread < thread_count; thread++)
   {
      tid[thread] = thread;
      pthread_create(&thread_handles[thread], NULL,
                     Pth_mat_vect2, &tid[thread]);
   }

   for (thread = 0; thread < thread_count; thread++)
      pthread_join(thread_handles[thread], NULL);

   free(thread_handles);
   free(tid);
} /* Mat_vect_mult_parallel */

void Print_matrix(char *title, double A[], int m, int n)
{
   int i, j;

   printf("%s\n", title);
   for (i = 0; i < m; i++)
   {
      for (j = 0; j < n; j++)
         printf("%4.1f ", A[i * n + j]);
      printf("\n");
   }
} 

void Print_vector(char *title, double y[], double m)
{
   int i;

   printf("%s\n", title);
   for (i = 0; i < m; i++)
      printf("%4.1f ", y[i]);
   printf("\n");
}

/*
   -> (m=8000, n=8000) - rezultatul paralel este mai rapid decat cel serial,
                         speedup-ul este si el decent, fiecare thread operand
                         pe o portiune din matrice;

   -> (m=8, n=8000000) - fiecare rand din matrice e mare, iar atunci cand accesam
                         elementele din matrice, avem cache miss-uri, iar vectorul 
                         x este mare (n elemente) si este accesat in mod repetat => 
                         poor cache utilization => timp paralel mai mic

   -> (m=8000000, n=8) - similar cu cazul anterior; acum fiecare coloana din matrice 
                         e mai mare, dar inmultirea cu vectorul x se face mai rapida
                         => better cache utilization => speedup-ul este mai mare decat
                         in cazul anterior

   Cand m=8 si n=8000000 (TP2 si S2):
   - fiecare rand e mare;
   - fiecare thread trb sa acceseze tot vectorul x de n elemente;
   - poor cache utilization and cache misses;
*/
