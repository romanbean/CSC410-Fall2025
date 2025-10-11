// Finding the average of an array - help needed with synchronization

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_THREADS 5
#define ARRAY_SIZE 20

int numbers[ARRAY_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20}; 
int partial_sums[NUM_THREADS] = {0}; 
pthread_barrier_t barrier; // Declare the barrier

void* func(void* arg) {
    int id = *((int*)arg);
    int segment_size = ARRAY_SIZE / NUM_THREADS; 
    int start = id * segment_size;          
    int end = (id == NUM_THREADS - 1) ? ARRAY_SIZE : start + segment_size;

    // Phase 1: Calculate the partial sum 
    printf("Thread %d: Calculating sum from index %d to %d.\n", id, start, end - 1);
    for (int i = start; i < end; i++) {
        partial_sums[id] += numbers[i];
    }
    printf("Thread %d: Partial sum is %d.\n", id, partial_sums[id]);

	// Wait for all threads to finish Phase 1
	pthread_barrier_wait(&barrier);

    // Phase 2: Calculate the total sum and average
    if (id == 0) { // Only the first thread calculates the final average
        int total_sum = 0;
        for (int i = 0; i < NUM_THREADS; i++) {
            total_sum += partial_sums[i];
        }
        double average = (double)total_sum / ARRAY_SIZE;
        printf("Total sum is: %d, Average is: %.2f\n", total_sum, average);
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS] = {0, 1, 2, 3, 4};

	// Initialize the barrier to synchronize NUM_THREADS threads
	pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, func, &thread_ids[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

	// Destroy the barrier after use
	pthread_barrier_destroy(&barrier);


    return 0;
}
