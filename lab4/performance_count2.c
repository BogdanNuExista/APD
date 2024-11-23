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
VERSION 2: Each thread works with a local variable lcount and at the end
merges its local result into the global counter
*/

#define NELEM 10000000
int a[NELEM];
int x = 3;
int count; // how many times appears x in array a?
pthread_mutex_t count_lock;

#define N_THREADS 8
int elem_per_thread = NELEM / N_THREADS;


// function to become body of thread 
// the argument will be thread id
// version 2: compute in local variable lcount
void* count_fct2(void* var)
{
	// thread id
	int id = *(int*)var;
	int start = id * elem_per_thread; //first elem to be processed
	int end = (id + 1) * elem_per_thread - 1; //last elem to be processes in this thread
	if (id == N_THREADS - 1) end = NELEM - 1;

	//printf("thread %d looks between index %d and %d \n", id, start, end);

	int lcount = 0;  // local counter 
	for (int i = start; i <= end; i++)
		if (a[i] == x) lcount++;

	//printf("thread %d has local count %d  \n", id,  lcount);    

	pthread_mutex_lock(&count_lock);
	count = count + lcount;
	pthread_mutex_unlock(&count_lock);

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


	for (int i = 0; i < NELEM; i++)
		a[i] = i % 4;

	int tid[N_THREADS];
	pthread_t thread[N_THREADS];

	int rc;

	elem_per_thread = NELEM / N_THREADS;

	struct timespec start, finish;
	double elapsed;

	//printf("start\n");
	clock_gettime(CLOCK_MONOTONIC, &start); // measure wall clock time!



	for (int i = 0; i < N_THREADS; i++) {
		tid[i] = i;

		rc = pthread_create(&thread[i], NULL, count_fct2, (void*)&tid[i]);
		if (rc) {
			printf("ERROR; pthread_create() return code is %d\n", rc);
			exit(-1);
		}

	}

	for (int i = 0; i < N_THREADS; i++) {
		// the main thread waits for thread1 to finish
		rc = pthread_join(thread[i], NULL);
		if (rc) {
			printf("ERROR; pthread_join() return code is %d\n", rc);
			exit(-1);
		}
	}


	clock_gettime(CLOCK_MONOTONIC, &finish);

	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

	printf("time paralel=%f \n", elapsed);

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