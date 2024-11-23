#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

/*
Parallel search: count how many times the value x appears in the array a.
The array a is divided in chunks that are given to N_THREADS threads.
Each chunk has elem_per_thread=NELEM/N_THREADS elements.
The thread with number id gets the chunk containing the elements 
between index start=id*elem_per_thread and end=(id+1)*elem_per_thread-1.
Each thread searches in its own chunk.
VERSION 1: Each thread works directly with a global count at every iteration
*/

#define NELEM 10000000  // number of elements
int a[NELEM];
int x = 3;   // value x to be searched for in array a
int count;   // how many times appears x in array a?
pthread_mutex_t count_lock;  // mutex protecting global variable count

#define N_THREADS 8    // number of threads
int elem_per_thread = NELEM / N_THREADS;  // elements per thread


// function to become body of thread 
// the argument is the thread id
// count_fct1 works directly with global variable count at every iteration
void* count_fct1(void* var)
{    
	int id = *(int*)var; // thread id
	int start = id * elem_per_thread; //first elem to be processed
	int end = (id + 1) * elem_per_thread - 1; //last elem to be processes in this thread
	if (id == N_THREADS - 1) end = NELEM - 1;

	//printf("thread %d looks between index %d and %d \n", id, start, end);

	for (int i = start; i <= end; i++)
		if (a[i] == x) {
                        // must use mutex lock at every occurence of x!
			pthread_mutex_lock(&count_lock); 
			count = count + 1;
			pthread_mutex_unlock(&count_lock);
		}
         return NULL;
}

void serial_count()
{
	count = 0;
	for (int i = 0; i < NELEM; i++)
		if (a[i] == x) count++;
}


int main(int argc, char** argv)
{

    // initialize values of array a
	// a: 0 1 2 3 0 1 2 3 0 1 2 3 ...
	for (int i = 0; i < NELEM; i++)
		a[i] = i % 4;

	int tid[N_THREADS];
	pthread_t thread[N_THREADS];

	int rc;

	//printf("start\n");

	struct timespec start, finish;
	double elapsed;

	clock_gettime(CLOCK_MONOTONIC, &start); // measure wall clock time!

	for (int i = 0; i < N_THREADS; i++) {
		tid[i] = i;
		rc = pthread_create(&thread[i], NULL, count_fct1, (void*)&tid[i]);
		if (rc) {
			printf("ERROR; pthread_create() return code is %d\n", rc);
			exit(-1);
		}
	}

	for (int i = 0; i < N_THREADS; i++) {
		// the main thread waits for thread[i] to finish
		rc = pthread_join(thread[i], NULL);
		if (rc) {
			printf("ERROR; pthread_join() return code is %d\n", rc);
			exit(-1);
		}
	}


	clock_gettime(CLOCK_MONOTONIC, &finish);

	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

	printf("time paralel =%f \n", elapsed);
	//printf("count=%d \n", count);

	clock_gettime(CLOCK_MONOTONIC, &start); // measure wall clock time!

	serial_count();

	clock_gettime(CLOCK_MONOTONIC, &finish);

	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

	printf("time serial=%f \n", elapsed);
	//printf("count=%d \n", count);

	return 0;
}