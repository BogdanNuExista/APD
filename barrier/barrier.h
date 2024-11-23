#ifndef BARRIER_H
#define BARRIER_H

#include <pthread.h>

typedef struct {
    int count;              // Number of threads that should wait at the barrier
    int current_count;      // Number of threads currently waiting
    pthread_mutex_t mutex;  // Mutex for protecting the counter
    pthread_cond_t cond;    // Condition variable for signaling
    int cycle;             // Used to handle multiple uses of the barrier
} my_barrier_t;

// Initialize the barrier with a given count
void my_barrier_init(my_barrier_t *b, int count);

// Destroy the barrier and free its resources
void my_barrier_destroy(my_barrier_t *b);

// Wait at the barrier until all threads arrive
void my_barrier_wait(my_barrier_t *b);

#endif // BARRIER_H