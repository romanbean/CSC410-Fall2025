#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define SIZE 100000000
#define NUM_THREADS 1

long long arr[SIZE];
//int partialSums[NUM_THREADS] = {0}; // Array to store partial sums for each thread
long long partialSums[NUM_THREADS] = {0};
// Entry function for each thread
void* sumPart(void* arg) 
{
	int thread_id = *(int*)arg;
	long long start = thread_id * (SIZE / NUM_THREADS);
	long long end = (thread_id == NUM_THREADS - 1) ? SIZE : start + (SIZE / NUM_THREADS);
	long long sum = 0;
	for (long long i = start; i < end; i++) {
		sum += arr[i];
	}
	partialSums[thread_id] = sum;
	return NULL;
}

int main() 
{
    // Initialize the array
    for (int i = 0; i < SIZE; i++) {
        arr[i] = i + 1; 
    }

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // Create threads to compute partial sums
	for (int i = 0; i < NUM_THREADS; i++) {
		thread_ids[i] = i;
		pthread_create(&threads[i], NULL, sumPart, &thread_ids[i]);
	}

    // Wait for all threads to finish
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

    // Combine the partial sums from all threads
    long long totalSum = 0;
    long long seqTotalSum = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        totalSum += partialSums[i];
    }

	// Sequential part
	clock_t start = clock();
	for (int i = 0; i < SIZE; i++) {
		seqTotalSum += arr[i];
	}

	clock_t end = clock();
	double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
	printf("Sequential sum time: %f seconds \n", time_spent);

    // Print the total sum;
    printf("Total Sum: %lld\n", totalSum);

    return 0;
}
