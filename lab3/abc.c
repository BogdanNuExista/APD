#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define REPEAT 100

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int turn=0; // 0 = A, 1 = B, 2 = C
int chars_printed = 0; // in exemplul de la exercitiu permutarile au 3 elemente 

void *HelloA(void *dummy)
{
    for(int i=0;i<REPEAT;i++)
    {
        pthread_mutex_lock(&mutex);
        while(turn!=0 || chars_printed == 3) {
            pthread_cond_wait(&cond, &mutex);
        }
        printf("A");
        chars_printed++;
        turn=(turn+1)%3;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *HelloB(void *dummy)
{
    for(int i=0;i<REPEAT;i++)
    {
        pthread_mutex_lock(&mutex);
        while(turn!=1 || chars_printed == 3) {
            pthread_cond_wait(&cond, &mutex);
        }
        printf("B");
        chars_printed++;
        turn=(turn+1)%3;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *HelloC(void *dummy)
{
    for(int i=0;i<REPEAT;i++)
    {
        pthread_mutex_lock(&mutex);
        while(turn!=2 || chars_printed == 3) {
            pthread_cond_wait(&cond, &mutex);
        }
        printf("C");
        chars_printed++;
        turn=(turn+1)%3;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t thread_handleA, thread_handleB, thread_handleC;

    srand(time(NULL));

    pthread_create(&thread_handleA, NULL, HelloA, NULL);
    pthread_create(&thread_handleB, NULL, HelloB, NULL);
    pthread_create(&thread_handleC, NULL, HelloC, NULL);

    for (int i = 0; i < REPEAT; i++)
    {
        pthread_mutex_lock(&mutex);
        while(chars_printed<3) {
            pthread_cond_wait(&cond, &mutex);
        }
        printf("\n");
        
        chars_printed=0;
        turn = rand()%3;

        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
     
    pthread_join(thread_handleA, NULL);
    pthread_join(thread_handleB, NULL);
    pthread_join(thread_handleC, NULL); 

    return 0;
}