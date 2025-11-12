#include <mpi.h>
#include "matrixMul.h"

int main(int argc, char* argv[]) {
    int rank, size;
    int n = N;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (n % size != 0) {
        if (rank == 0)
            printf("Matrix size %d not divisible by number of processes %d!\n", n, size);
        MPI_Finalize();
        return 0;
    }

    int rows_per_proc = n / size;

    // Allocate memory
    int* A = NULL;
    int* B = (int*)malloc(n * n * sizeof(int));
    int* C = NULL;

    if (rank == 0) {
        A = (int*)malloc(n * n * sizeof(int));
        C = (int*)malloc(n * n * sizeof(int));

        // Initialize matrices
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j) {
                A[i * n + j] = 1;
                B[i * n + j] = 1;
            }
    }

    int* local_A = (int*)malloc(rows_per_proc * n * sizeof(int));
    int* local_C = (int*)malloc(rows_per_proc * n * sizeof(int));

    // Start timing
    double start_time = MPI_Wtime();

    // Broadcast B to all processes
    MPI_Bcast(B, n * n, MPI_INT, 0, MPI_COMM_WORLD);

    // Scatter A
    MPI_Scatter(A, rows_per_proc * n, MPI_INT,
                local_A, rows_per_proc * n, MPI_INT, 0, MPI_COMM_WORLD);

    // Local matrix multiplication
    for (int i = 0; i < rows_per_proc; i++) {
        for (int j = 0; j < n; j++) {
            int sum = 0;
            for (int k = 0; k < n; k++)
                sum += local_A[i * n + k] * B[k * n + j];
            local_C[i * n + j] = sum;
        }
    }

    // Gather results
    MPI_Gather(local_C, rows_per_proc * n, MPI_INT,
               C, rows_per_proc * n, MPI_INT, 0, MPI_COMM_WORLD);

    // End timing
    double end_time = MPI_Wtime();
    double elapsed = end_time - start_time;

    // Print result and timing
    if (rank == 0) {
        printf("Parallel Matrix Multiplication Complete!\n");
        printf("Matrix size: %dx%d, Processes: %d\n", n, n, size);
        printf("Elapsed time: %f seconds\n", elapsed);

        // Optional correctness check
        // printf("Sample result (C[0][0]) = %d\n", C[0]);
    }

    // Free memory
    free(local_A);
    free(local_C);
    free(B);
    if (rank == 0) {
        free(A);
        free(C);
    }

    MPI_Finalize();
    return 0;
}
