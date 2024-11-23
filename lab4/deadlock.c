#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define REPEAT 10
#define N_THREADS 2

pthread_mutex_t fork_lock;
pthread_mutex_t knife_lock;

void *eat_fct(void *var)
{
        int id = *(int *)var; 

        for (int i = 0; i < REPEAT; i++)
        {
            printf("Thread %d wants fork \n",id);
            pthread_mutex_lock(&fork_lock);
            printf("Thread %d has fork wants knife \n",id);
            pthread_mutex_lock(&knife_lock);
            // uses fork and knife
            sleep(1);
            printf("Thread %d has fork and knife \n",id);
            pthread_mutex_unlock(&knife_lock);
            pthread_mutex_unlock(&fork_lock);
        }

        return NULL;
}

int main(int argc, char **argv)
{

    int tid[N_THREADS];
    pthread_t thread[N_THREADS];
    int rc;

    pthread_mutex_init(&fork_lock, NULL);
    pthread_mutex_init(&knife_lock, NULL);

    printf("start\n");

    for (int i = 0; i < N_THREADS; i++)
    {
        tid[i] = i;
        rc = pthread_create(&thread[i], NULL, eat_fct, (void *)&tid[i]);
        if(rc)
        {
            printf("ERROR; pthread_create() return code is %d\n", rc);
            exit(-1);
        }
    }

    for (int i = 0; i < N_THREADS; i++)
    {
        rc = pthread_join(thread[i], NULL);
        if (rc)
        {
                printf("ERROR; pthread_join() return code is %d\n", rc);
                exit(-1);
        }
    }

    pthread_mutex_destroy(&fork_lock);
    pthread_mutex_destroy(&knife_lock);

    return 0;
}