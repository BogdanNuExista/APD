#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define REPEAT 5    
#define NUM_THREADS 4

typedef struct node
{
    int data;
    struct node *next;
} queueNode;

queueNode *head = NULL;
queueNode *tail = NULL;

pthread_mutex_t lock;
pthread_cond_t not_empty_cv;

int isEmpty(void)
{
    return (head == NULL);
}

void put(int val)
{
    queueNode *new = malloc(sizeof(queueNode));
    new->data = val;
    new->next = NULL;

    if (!isEmpty())
    {
        tail->next = new;
        tail = new;
    }
    else
    {
        head = new;
        tail = new;
    }
}

int get()
{
    if (isEmpty())
    {
        printf("Error! empty queue \n");
        exit(1);
    }
    queueNode *oldhead = head;
    int rez = oldhead->data;
    head = head->next;
    if (head == NULL)
        tail = NULL;
    free(oldhead);
    return rez;
}

void init_synchro()
{
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&not_empty_cv, NULL);
}

void destroy_synchro()
{
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&not_empty_cv);
}

void *producer(void *t)
{
    int i;
    int my_id = *(int *)t;

    for (i = 0; i < REPEAT; i++)
    {
        pthread_mutex_lock(&lock);
        put(i+my_id*10);
        printf("Producer thread %d produced %d \n",my_id,i+my_id*10);
        pthread_cond_signal(&not_empty_cv);
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
}

void *consumer(void *t)
{
    int i;
    int my_id = *(int *)t;

    for (i = 0; i < REPEAT; i++)
    {
        pthread_mutex_lock(&lock);
        while (isEmpty())
        {
            pthread_cond_wait(&not_empty_cv, &lock);
        }
        int rez = get();
        printf("Consumer thread %d consumed %d \n",my_id,rez);
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    init_synchro();

    for (int i = 0; i < NUM_THREADS; i++)
    {
        ids[i] = i;
        if (i % 2 == 0)
            pthread_create(&threads[i], NULL, producer, (void *)&ids[i]);
        else
            pthread_create(&threads[i], NULL, consumer, (void *)&ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    destroy_synchro();

    return 0;
}