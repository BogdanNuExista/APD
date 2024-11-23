/**
 * File:    omp_odd_even.c
 * 
 * adapts code examples from [pacheco]
 * IPP2:  Section 5.6.2
 * 
 * Version1: forks and joins threads in each iteration
 * Version2: forks and joins the threads only once.
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <string.h>

int thread_count;

void Usage(char *prog_name);
void Get_args(int argc, char *argv[], int *n_p);
void Generate_list(int a[], int n);
void Copy_list(int a[], int b[], int n);
void Print_list(int a[], int n, char *title);
void Read_list(int a[], int n);

void Odd_even_serial(int a[], int n);
void Odd_even_v1(int a[], int n);
void Odd_even_v2(int a[], int n);


////////////////////////////////////////////////////////////////////////////////////////////

// functia e identica cu cea din omp_countsort_skel.c, doar ca am adaugat acea directiva 

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

    // punem vectorul sortat inapoi in a
    memcpy(a, temp, n * sizeof(int));
    free(temp);
}


////////////////////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
   int n;
   char g_i;
   int *a;
   int *a1;
   double start, finish;

   Get_args(argc, argv, &n); 
   a = malloc(n * sizeof(int));
   a1 = malloc(n * sizeof(int));

   Generate_list(a, n);
   printf("Finished list generation \n");
#ifdef DEBUG
   Print_list(a, n, "Before sort");
#endif

   Copy_list(a, a1, n);
   printf("Sorting parallel V1 ...");
   start = omp_get_wtime();
   Odd_even_v1(a, n);
   finish = omp_get_wtime();

#ifdef DEBUG
   Print_list(a, n, "After sort");
#endif

   printf("Elapsed time paralel odd-even sort v1 = %5.4f seconds\n", finish - start);

   Copy_list(a1, a, n);
   printf("Sorting parallel V2 ...");
   start = omp_get_wtime();
   Odd_even_v2(a, n);
   finish = omp_get_wtime();

#ifdef DEBUG
   Print_list(a, n, "After sort");
#endif

   printf("Elapsed time paralel odd-even sort v2 = %5.4f seconds\n", finish - start);

   Copy_list(a1, a, n); // restore original list


   ////////////////////////////////////////////////////////////////////////////////////////////
   printf("Sorting parallel Countsort ...");
   start = omp_get_wtime();
   Countsort_parallel(a, n);
   finish = omp_get_wtime();

#ifdef DEBUG
   Print_list(a, n, "After sort");
#endif

   printf("Elapsed time paralel countsort = %5.4f seconds\n", finish - start);
   ////////////////////////////////////////////////////////////////////////////////////////////


   Copy_list(a1, a, n);




   printf("Sorting Serial ...");
   start = omp_get_wtime();
   Odd_even_serial(a, n);
   finish = omp_get_wtime();

#ifdef DEBUG
   Print_list(a, n, "After sort");
#endif

   printf("Elapsed time Serial odd-even sort serial = %5.4f seconds\n", finish - start);

   free(a);
   free(a1);
   return 0;
} /* main */

/*-----------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Summary of how to run program
 */
void Usage(char *prog_name)
{
   fprintf(stderr, "usage:   %s <thread count> <n> \n", prog_name);
   fprintf(stderr, "   n:   number of elements in list\n");
} /* Usage */

/*-----------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get and check command line arguments
 * In args:   argc, argv
 * Out args:  n_p
 */
void Get_args(int argc, char *argv[], int *n_p)
{
   if (argc != 3)
   {
      Usage(argv[0]);
      exit(0);
   }
   thread_count = strtol(argv[1], NULL, 10);
   *n_p = strtol(argv[2], NULL, 10);

   if (*n_p <= 0)
   {
      Usage(argv[0]);
      exit(0);
   }
} /* Get_args */

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

/*-----------------------------------------------------------------
 * Function:     Bubblesort
 * Purpose:      Classic serial Bubblesort
 * In args:      n
 * In/out args:  a
 */
void Bubblesort_serial(int a[], int n)
{
   int list_length, i;
   double tmp;

   for (list_length = n; list_length >= 2; list_length--)
      for (i = 0; i < list_length - 1; i++)
      {
         if (a[i] > a[i + 1])
         {
            tmp = a[i + 1];
            a[i + 1] = a[i];
            a[i] = tmp;
         }
      }
} /* Bubblesort */

/*-----------------------------------------------------------------
 * Function:     Odd_even_serial
 * Purpose:      Sort list using odd-even transposition sort
 * In args:      n
 * In/out args:  a
 */
void Odd_even_serial(int a[], int n)
{
   int phase, i, tmp;

   for (phase = 0; phase < n; phase++)
   {
      if (phase % 2 == 0)

         for (i = 1; i < n; i += 2)
         {
            if (a[i - 1] > a[i])
            {
               tmp = a[i - 1];
               a[i - 1] = a[i];
               a[i] = tmp;
            }
         }
      else
         for (i = 1; i < n - 1; i += 2)
         {
            if (a[i] > a[i + 1])
            {
               tmp = a[i + 1];
               a[i + 1] = a[i];
               a[i] = tmp;
            }
         }
   }
} /* Odd_even serial */

/*-----------------------------------------------------------------
 * Function:     Odd_even_v1
 * Purpose:      Sort list using odd-even transposition sort
 * Version:      V1 - fork join threads at every iteration
 * In args:      n
 * In/out args:  a
 */
void Odd_even_v1(int a[], int n)
{
   int phase, i, tmp;
   for (phase = 0; phase < n; phase++)
   {
      if (phase % 2 == 0)
#pragma omp parallel for num_threads(thread_count) default(none) shared(a, n) private(i, tmp)
         for (i = 1; i < n; i += 2)
         {
            if (a[i - 1] > a[i])
            {
               tmp = a[i - 1];
               a[i - 1] = a[i];
               a[i] = tmp;
            }
         }
      else
#pragma omp parallel for num_threads(thread_count) default(none) shared(a, n) private(i, tmp)
         for (i = 1; i < n - 1; i += 2)
         {
            if (a[i] > a[i + 1])
            {
               tmp = a[i + 1];
               a[i + 1] = a[i];
               a[i] = tmp;
            }
         }
   }
} /* Odd_even v1 */

/*-----------------------------------------------------------------
 * Function:     Odd_even_v2
 * Purpose:      Sort list using odd-even transposition sort
 * Version:      V2 - fork join threads only once
 * In args:      n
 * In/out args:  a
 */
void Odd_even_v2(int a[], int n)
{
   int phase, i, tmp;

#pragma omp parallel num_threads(thread_count) default(none) shared(a, n) private(i, tmp, phase)
   for (phase = 0; phase < n; phase++)
   {
      if (phase % 2 == 0)
#pragma omp for
         for (i = 1; i < n; i += 2)
         {
            if (a[i - 1] > a[i])
            {
               tmp = a[i - 1];
               a[i - 1] = a[i];
               a[i] = tmp;
            }
         }
      else
#pragma omp for
         for (i = 1; i < n - 1; i += 2)
         {
            if (a[i] > a[i + 1])
            {
               tmp = a[i + 1];
               a[i + 1] = a[i];
               a[i] = tmp;
            }
         }
   }
} /* Odd_even v2 */