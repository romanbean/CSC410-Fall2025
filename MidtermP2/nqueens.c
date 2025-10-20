#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#define N 15
#define K 2 // depth of partial exploration before spawning threads
#define MAX_THREADS 1000
#define MAX_PARALLEL_THREADS 4
#define RUN_SEQUENTIAL 1 // 0 for parallel
 
int total_solutions = 0;
pthread_mutex_t total_mutex;

typedef struct {
    int partial_board[N];
    int col;
} thread_arg_t;

// Check if it's safe to place a queen at (row, col)
bool isSafe(int board[], int row, int col) {
    for (int i = 0; i < col; i++) {
        if (board[i] == row || abs(board[i] - row) == col - i)
            return false;
    }
    return true;
}

// Recursive backtracking solver
void solve(int board[], int col, int* count) {
    if (col == N) {
        (*count)++;
        return;
    }

    for (int row = 0; row < N; row++) {
        if (isSafe(board, row, col)) {
            board[col] = row;
            solve(board, col + 1, count);
        }
    }
}

// Thread worker
void* thread_worker(void* arg) {
    thread_arg_t* data = (thread_arg_t*)arg;
    int count = 0;
    int board[N];
    memcpy(board, data->partial_board, sizeof(board));

    solve(board, data->col, &count);

    // Safely update global solution count
    pthread_mutex_lock(&total_mutex);
    total_solutions += count;
    pthread_mutex_unlock(&total_mutex);

    free(arg);
    return NULL;
}

void spawn_threads() {
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;

    int board[N];

    // Loop through partial solutions (K = 2 for row1 and row2)
    for (int row1 = 0; row1 < N; row1++) {
        board[0] = row1;
        for (int row2 = 0; row2 < N; row2++) {
            board[1] = row2;
            if (isSafe(board, row2, 1)) {
                if (thread_count >= MAX_PARALLEL_THREADS) {
                    // Stop spawning more threads
                    break;
                }

                thread_arg_t* arg = malloc(sizeof(thread_arg_t));
                memcpy(arg->partial_board, board, sizeof(board));
                arg->col = 2; // because K = 2

                pthread_create(&threads[thread_count++], NULL, thread_worker, arg);
            }
        }

        if (thread_count >= MAX_PARALLEL_THREADS)
            break;
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}


int main() {
    clock_t start = clock();

    pthread_mutex_init(&total_mutex, NULL);

    if (RUN_SEQUENTIAL) {
        int board[N];
        for (int i = 0; i < N; i++) board[i] = -1;

        int count = 0;
        solve(board, 0, &count);

        total_solutions = count;
    } else {
        spawn_threads();  // Run the parallel version
    }

    pthread_mutex_destroy(&total_mutex);

    clock_t end = clock();
    double duration = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Total solutions for N=%d: %d\n", N, total_solutions);
    if (RUN_SEQUENTIAL)
        printf("Execution time (sequential): %.4f seconds\n", duration);
    else
        printf("Execution time (parallel with %d threads): %.4f seconds\n", MAX_PARALLEL_THREADS, duration);

    return 0;
}

