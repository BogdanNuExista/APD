#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 2
#define NUM_INTERATIONS 1000000

pthread_mutex_t mutex;

int count = 0;

void *inc_count(void *t)
{
    int my_id = *(int *)t;
    for(int i=0;i<NUM_INTERATIONS;i++)
    {
        pthread_mutex_lock(&mutex);
        count++;
        pthread_mutex_unlock(&mutex);
    }
    printf("Threadul %d a terminat de incrementat count-ul\n", my_id);
    return NULL;
}

/*
    Minimum and maximum possible values: (fara sa folosim mutex, acesta asigura faptul
                                          ca fiecare incrementare este atomica, deci 
                                          count-ul va fi incrementat corect la 2000000)

    -> valoarea minima posibila este 1000000, deoarece este posibil ca incrementarile 
       facute de thread-uri sa-si dea overlap, iar unele increment-uri sa fie pierdute 
       din cauza la race conditions.
    -> valoarea maxima posibila este 2000000, deoarece este posibil ca incrementarile 
       facute de thread-uri sa nu-si dea overlap, si deci pentru thread-ul 1 sa ajunga
       count-ul la 1000000, iar pentru thread-ul 2 sa ajunga count-ul la 2000000.
*/

int main(int argc, char *argv[])
{
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < NUM_THREADS; i++)
    {
        ids[i] = i;
        pthread_create(&threads[i], NULL, inc_count, (void *)&ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("Final value of count = %d \n", count);

    pthread_mutex_destroy(&mutex);

    return 0;
}