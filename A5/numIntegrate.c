#include <stdio.h>
#include <mpi.h>

#define N 1000000000  // you can reduce from 100,000,000,000 for testing first

double f(double x) {
    return 4.0 / (1.0 + x * x);  // Function to integrate (approximates π)
}

int main(int argc, char *argv[]) {
    int rank, size;
    double a = 0.0, b = 1.0;  // Integration limits
    long long n = N;          // Number of trapezoids

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double start_time = MPI_Wtime();

    // Step 1: Each process computes its local interval
    double h = (b - a) / n;                     // Step size
    long long local_n = n / size;               // Intervals per process
    double local_a = a + rank * local_n * h;    // Start point
    double local_b = local_a + local_n * h;     // End point

    // Step 2: Local trapezoidal integration
    double local_sum = (f(local_a) + f(local_b)) / 2.0;
    for (long long i = 1; i < local_n; i++) {
        double x = local_a + i * h;
        local_sum += f(x);
    }
    local_sum *= h;

    // Step 3: Reduce all local results to rank 0
    double total_sum = 0.0;
    MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();

    // Step 4: Print result
    if (rank == 0) {
        printf("Parallel Trapezoidal Integration Complete!\n");
        printf("Estimated value of π: %.15f\n", total_sum);
        printf("Elapsed time: %f seconds\n", end_time - start_time);
    }

    MPI_Finalize();
    return 0;
}
