#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N 1000  // Size of the matrix
#define NUM_THREADS 1  // Number of threads

int **A, **B, **C;  // Global matrices

// Structure to hold information for each thread
typedef struct 
{
    int thread_id;
    int num_rows;  // Number of rows each thread will handle
} thread_data_t;

// Function for each thread to perform matrix multiplication
void* matrixMultiplyThread(void* arg) 
{
    // Divide the task (rows) of each thread based on thread id
    // compute a portion of the matrix multiplication
	thread_data_t* data = (thread_data_t*)arg;
	int thread_id = data->thread_id;
	int rows_per_thread = data->num_rows;
	int start_row = thread_id * rows_per_thread;
	int end_row = (thread_id == NUM_THREADS - 1) ? N : start_row + rows_per_thread;

	printf("Thread %d processing rows %d to %d\n", thread_id, start_row, end_row - 1);

	for (int i = start_row; i < end_row; i++) {
		for (int j = 0; j < N; j++) {
			C[i][j] = 0;
			for (int k = 0; k < N; k++) {
				C[i][j] += A[i][k] * B[k][j];
			}
		}
	}

	printf("Thread %d finished\n", thread_id);
	pthread_exit(NULL);
}

void displayMatrix(int** matrix, int n) 
{
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}
