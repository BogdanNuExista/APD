#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "barrier.h"

// Configurable parameters
#define N 1000000  // Number of floating point operations in do_work

// These will be set via command line arguments
int NUMTHREADS;
int REPEATS;

// Barriers (we'll use both to compare)
pthread_barrier_t pthread_barrier;
my_barrier_t my_barrier;

// Function to create some workload
void do_work(int thread_id) {
    float sum = 0.0;
    for (int i = 0; i < N; i++) {
        sum += (float)i * thread_id;
    }
}

// Thread function for pthread_barrier test
void* pthread_barrier_thread(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < REPEATS; i++) {
        do_work(thread_id);
        pthread_barrier_wait(&pthread_barrier);
    }
    
    return NULL;
}

// Thread function for my_barrier test
void* my_barrier_thread(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < REPEATS; i++) {
        do_work(thread_id);
        my_barrier_wait(&my_barrier);
    }
    
    return NULL;
}

// Function to run benchmark with specified barrier type
double run_benchmark(int use_pthread_barrier) {
    pthread_t* threads = malloc(NUMTHREADS * sizeof(pthread_t));
    int* thread_ids = malloc(NUMTHREADS * sizeof(int));
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < NUMTHREADS; i++) {
        thread_ids[i] = i;
        if (use_pthread_barrier) {
            pthread_create(&threads[i], NULL, pthread_barrier_thread, &thread_ids[i]);
        } else {
            pthread_create(&threads[i], NULL, my_barrier_thread, &thread_ids[i]);
        }
    }
    
    for (int i = 0; i < NUMTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    free(threads);
    free(thread_ids);
    
    double time_taken = (end.tv_sec - start.tv_sec) +
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    return time_taken;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s NUMTHREADS REPEATS NUM_EXPERIMENTS\n", argv[0]);
        return 1;
    }
    
    NUMTHREADS = atoi(argv[1]);
    REPEATS = atoi(argv[2]);
    int num_experiments = atoi(argv[3]);
    
    double* pthread_times = malloc(num_experiments * sizeof(double));
    double* my_times = malloc(num_experiments * sizeof(double));
    
    printf("Running benchmark with:\n");
    printf("NUMTHREADS: %d\n", NUMTHREADS);
    printf("REPEATS: %d\n", REPEATS);
    printf("Number of experiments: %d\n\n", num_experiments);
    
    // Run experiments
    for (int i = 0; i < num_experiments; i++) {
        // Initialize barriers
        pthread_barrier_init(&pthread_barrier, NULL, NUMTHREADS);
        my_barrier_init(&my_barrier, NUMTHREADS);
        
        // Run benchmarks
        pthread_times[i] = run_benchmark(1);
        my_times[i] = run_benchmark(0);
        
        // Cleanup
        pthread_barrier_destroy(&pthread_barrier);
        my_barrier_destroy(&my_barrier);
        
        printf("Experiment %d:\n", i + 1);
        printf("pthread_barrier time: %.6f seconds\n", pthread_times[i]);
        printf("my_barrier time: %.6f seconds\n\n", my_times[i]);
    }
    
    // Calculate averages
    double pthread_avg = 0, my_avg = 0;
    for (int i = 0; i < num_experiments; i++) {
        pthread_avg += pthread_times[i];
        my_avg += my_times[i];
    }
    pthread_avg /= num_experiments;
    my_avg /= num_experiments;
    
    printf("Average Results:\n");
    printf("pthread_barrier average time: %.6f seconds\n", pthread_avg);
    printf("my_barrier average time: %.6f seconds\n", my_avg);
    printf("Difference (my_barrier - pthread_barrier): %.6f seconds\n", my_avg - pthread_avg);
    
    free(pthread_times);
    free(my_times);
    
    return 0;
}