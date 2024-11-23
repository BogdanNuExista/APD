#include "barrier.h"
#include <stdlib.h>

void my_barrier_init(my_barrier_t *b, int count) {
    b->count = count;
    b->current_count = 0;
    b->cycle = 0;
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
}

void my_barrier_destroy(my_barrier_t *b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

void my_barrier_wait(my_barrier_t *b) {
    pthread_mutex_lock(&b->mutex);
    
    int my_cycle = b->cycle;  // Remember which cycle we're in
    b->current_count++;
    
    if (b->current_count == b->count) {
        // Last thread to arrive
        b->cycle++;           // Increment the cycle
        b->current_count = 0; // Reset for next use
        pthread_cond_broadcast(&b->cond);
    } else {
        // Not the last thread, wait for others
        while (my_cycle == b->cycle) {
            pthread_cond_wait(&b->cond, &b->mutex);
        }
    }
    
    pthread_mutex_unlock(&b->mutex);
}