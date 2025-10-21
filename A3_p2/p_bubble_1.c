// ONE APPRAOCH OF PARALLELIZING THE ORIGINAL BUBBLE SORT

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 4
#define SIZE 100000
#define MAX_VAL 100000  // max random number

int array[SIZE];
pthread_barrier_t barrier;

typedef struct {
    int start, end;
} ThreadData;

void printArray() 
{
    for (int i = 0; i < SIZE; i++) 
        printf("%d ", array[i]);
    printf("\n");
}

void *parallel_bubble(void *arg) 
{
    ThreadData *data = (ThreadData *)arg;
    int start = data->start;
    int end = data->end;

    for (int i = 0; i < SIZE - 1; i++) {
        for (int j = start; j < end - 1; j++) {
            if (array[j] > array[j + 1]) {
                int tmp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = tmp;
            }
        }

        if (data->end < SIZE) {
            int j = end - 1;
            if (array[j] > array[j + 1]) {
                int tmp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = tmp;
            }
        }

        pthread_barrier_wait(&barrier);
    }

    return NULL;
}

int main() 
{
    for (int i = 0; i < SIZE; i++)
        array[i] = rand() % MAX_VAL;

    printf("Unsorted: ");
    printArray();

    pthread_t threads[NUM_THREADS];
    ThreadData tdata[NUM_THREADS];
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    int chunk_size = SIZE / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; i++) {
        tdata[i].start = i * chunk_size;
        tdata[i].end = (i == NUM_THREADS - 1) ? SIZE : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, parallel_bubble, &tdata[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    pthread_barrier_destroy(&barrier);

    printf("Sorted:   ");
    printArray();

    return 0;
}
