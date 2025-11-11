#include <mpi.h>
#include "matrixMul.h"

int main(int argc, char* argv[]) {
    int rank, size;
    int n = N;
    int** A = NULL;
    int** B = NULL;
    int** C = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rows_per_proc = n / size;

    // Allocate full matrices in root process
    if (rank == 0) {
        A = (int**)malloc(n * sizeof(int*));
        B = (int**)malloc(n * sizeof(int*));
        C = (int**)malloc(n * sizeof(int*));
        for (int i = 0; i < n; ++i) {
            A[i] = (int*)malloc(n * sizeof(int));
            B[i] = (int*)malloc(n * sizeof(int));
            C[i] = (int*)malloc(n * sizeof(int));
        }

        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j) {
                A[i][j] = 1;
                B[i][j] = 1;
                C[i][j] = 0;
            }
    }

    // Allocate space for local matrices
    int** local_A = (int**)malloc(rows_per_proc * sizeof(int*));
    int** local_C = (int**)malloc(rows_per_proc * sizeof(int*));
    for (int i = 0; i < rows_per_proc; ++i) {
        local_A[i] = (int*)malloc(n * sizeof(int));
        local_C[i] = (int*)malloc(n * sizeof(int));
    }

    // Allocate and broadcast matrix B
    if (rank != 0)
        B = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; ++i)
        if (rank != 0)
            B[i] = (int*)malloc(n * sizeof(int));

    // Send B matrix to all processes
    for (int i = 0; i < n; i++)
        MPI_Bcast(B[i], n, MPI_INT, 0, MPI_COMM_WORLD);

    // Scatter A among all processes
    for (int i = 0; i < rows_per_proc; i++) {
        MPI_Scatter(A ? A[i] : NULL, n, MPI_INT,
                    local_A[i], n, MPI_INT, 0, MPI_COMM_WORLD);
    }

    // Perform partial matrix multiplication
    for (int i = 0; i < rows_per_proc; i++) {
        for (int j = 0; j < n; j++) {
            local_C[i][j] = 0;
            for (int k = 0; k < n; k++)
                local_C[i][j] += local_A[i][k] * B[k][j];
        }
    }

    // Gather results to root process
    for (int i = 0; i < rows_per_proc; i++) {
        MPI_Gather(local_C[i], n, MPI_INT,
                   C ? C[i] : NULL, n, MPI_INT, 0, MPI_COMM_WORLD);
    }

    if (rank == 0) {
        printf("Parallel Matrix Multiplication Complete!\n");
        // displayMatrix(C, n); // Uncomment to display
    }

    // Cleanup
    for (int i = 0; i < rows_per_proc; ++i) {
        free(local_A[i]);
        free(local_C[i]);
    }
    free(local_A);
    free(local_C);
    if (rank == 0) {
        for (int i = 0; i < n; i++) {
            free(A[i]);
            free(B[i]);
            free(C[i]);
        }
        free(A);
        free(B);
        free(C);
    }

    MPI_Finalize();
    return 0;
}
