// ANOTHER APPROACH OF PARALLELIZING BUBBLE SORT (USES A VARIATION OF BUBBLE SORT - ODD/EVEN / BRICK SORT)

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 4
#define SIZE 100000
#define MAX_VAL 100000  // max random number

int *array;
int array_size;
pthread_barrier_t barrier;

void printArray(int* arr, int n) 
{
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");
}

void *parallel_odd_even(void *arg) 
{
    long thread_id = *(long *)arg;
    free(arg);

    int chunk_size = array_size / NUM_THREADS;
    int start_index = thread_id * chunk_size;
    int end_index = (thread_id == NUM_THREADS - 1) ? array_size : start_index + chunk_size;

    for (int phase = 0; phase < array_size; ++phase) {
        int is_even_phase = (phase % 2 == 0);

        for (int j = start_index + is_even_phase; j < end_index - 1; j += 2) {
            if (array[j] > array[j + 1]) {
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }

        if (thread_id != NUM_THREADS - 1) {
            int j = end_index - 1;
            if ((j % 2 == is_even_phase) && array[j] > array[j + 1]) {
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }

        pthread_barrier_wait(&barrier);
    }

    return NULL;
}

int main() 
{
    srand(time(NULL));
    array_size = SIZE;
    array = malloc(array_size * sizeof(int));
    if (!array) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    printf("Unsorted array: ");
    for (int i = 0; i < SIZE; i++) {
        array[i] = rand() % MAX_VAL;
        printf("%d ", array[i]);
    }
    printf("\n");


    if (pthread_barrier_init(&barrier, NULL, NUM_THREADS)) {
        perror("Barrier init failed");
        free(array);
        exit(EXIT_FAILURE);
    }

    pthread_t threads[NUM_THREADS];
    for (long i = 0; i < NUM_THREADS; i++) {
        long *tid = malloc(sizeof(long));
        if (!tid) 
        { 
            perror("malloc"); 
            exit(EXIT_FAILURE); 
        }
        *tid = i;
        if (pthread_create(&threads[i], NULL, parallel_odd_even, tid) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    pthread_barrier_destroy(&barrier);

    printf("Sorted array:   ");
    printArray(array, SIZE);

    free(array);
    return 0;
}
