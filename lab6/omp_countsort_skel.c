
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <omp.h>

//#define DEBUG

int thread_count;


void Generate_list(int a[], int n);
void Copy_list(int a[], int b[], int n);
void Print_list(int a[], int n, char *title);
void Read_list(int a[], int n);



void Countsort_serial(int a[], int n)
{
    int i, j, count;
    int *temp = malloc(n * sizeof(int));

    for (i = 0; i < n; i++)
    {
        count = 0;
        for (j = 0; j < n; j++)
            if(a[j] < a[i])
                count++;
        else if (a[j] == a[i] && j < i)
            count++;
        temp[count] = a[i];
    }
    memcpy(a, temp, n * sizeof(int));
    free(temp);
}


void Countsort_parallel(int a[], int n)
{
    int i, j, count;
    int *temp = malloc(n * sizeof(int));

    #pragma omp parallel for num_threads(thread_count) private(i, j, count) shared(a, temp, n)
    for (i = 0; i < n; i++)
    {
        count = 0;
        for (j = 0; j < n; j++)
        {
            if (a[j] < a[i])
                count++;
            else if (a[j] == a[i] && j < i)
                count++;
        }
        temp[count] = a[i];
    }

    memcpy(a, temp, n * sizeof(int));
    free(temp);
}


/*-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
   int n;
   char g_i;
   int *a;
   int *a1;
   double start, finish;

   
if (argc != 3)
   {
      printf("Usage: %s  <numthreads> <numelements>\n", argv[0]);
      exit(0);
   }
   thread_count = strtol(argv[1], NULL, 10);
   n = strtol(argv[2], NULL, 10);

   a = malloc(n * sizeof(int));
   a1 = malloc(n * sizeof(int));

   Generate_list(a, n);
   printf("Finished list generation \n");
#ifdef DEBUG
   Print_list(a, n, "Before sort");
#endif

   Copy_list(a, a1, n); // keep a copy of original list to repeat sorting

   printf("Sorting serial ...\n");
   start = omp_get_wtime();
   Countsort_serial(a,n);
   finish = omp_get_wtime();

#ifdef DEBUG
   Print_list(a, n, "After sort");
#endif

   printf("Elapsed time Serial Countsort v1 = %5.4f seconds\n", finish - start);

Copy_list(a1, a, n); // restore original to repeat sorting

   printf("Sorting parallel ...\n");
   start = omp_get_wtime();
   Countsort_parallel(a,n);
   finish = omp_get_wtime();

#ifdef DEBUG
   Print_list(a, n, "After sort");
#endif

   printf("Elapsed time Parallel Countsort v1 = %5.4f seconds\n", finish - start);


   free(a);
   free(a1);
   return 0;
} /* main */


/*-----------------------------------------------------------------
 * Function:  Generate_list
 * Purpose:   Use random number generator to generate list elements
 * In args:   n
 * Out args:  a
 */
void Generate_list(int a[], int n)
{
   int i;

   srand(time(NULL));
   for (i = 0; i < n; i++)
   {
      a[i] = rand() % n;
   }
} /* Generate_list */


/*-----------------------------------------------------------------
 * Function:  copy_list
 * Purpose:   Copies the elements in the list a to b
 * In args:   a, n
 * Out args:   b
 */
void Copy_list(int a[], int b[], int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      b[i] = a[i];
   }
} /* Copy_list */


/*-----------------------------------------------------------------
 * Function:  Print_list
 * Purpose:   Print the elements in the list
 * In args:   a, n
 */
void Print_list(int a[], int n, char *title)
{
   int i;

   printf("%s:\n", title);
   for (i = 0; i < n; i++)
      printf("%d ", a[i]);
   printf("\n\n");
} /* Print_list */
