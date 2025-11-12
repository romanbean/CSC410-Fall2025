#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <mpi.h>

void sieve_base_primes(int limit, bool *is_prime) {
    for (int i = 0; i <= limit; i++)
        is_prime[i] = true;

    is_prime[0] = is_prime[1] = false;

    for (int p = 2; p * p <= limit; p++) {
        if (is_prime[p]) {
            for (int i = p * p; i <= limit; i += p)
                is_prime[i] = false;
        }
    }
}

int main(int argc, char *argv[]) {
    int rank, size;
    long long n = 10000000;  // Range up to 10 million (adjustable)

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double start_time = MPI_Wtime();

    long long sqrt_n = (long long)sqrt(n);
    bool *base_primes = NULL;

    // Step 1: Rank 0 computes base primes up to sqrt(n)
    if (rank == 0) {
        base_primes = (bool *)malloc((sqrt_n + 1) * sizeof(bool));
        sieve_base_primes(sqrt_n, base_primes);
    }

    // Step 2: Broadcast base primes to all processes
    if (rank != 0)
        base_primes = (bool *)malloc((sqrt_n + 1) * sizeof(bool));
    MPI_Bcast(base_primes, sqrt_n + 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);

    // Step 3: Each process handles its subrange
    long long low = rank * (n / size) + 2;
    long long high = (rank == size - 1) ? n : (rank + 1) * (n / size) + 1;
    long long size_local = high - low + 1;

    bool *is_prime_local = (bool *)malloc(size_local * sizeof(bool));
    for (long long i = 0; i < size_local; i++)
        is_prime_local[i] = true;

    // Step 4: Mark non-primes in local range using base primes
    for (long long p = 2; p <= sqrt_n; p++) {
        if (base_primes[p]) {
            // Find the first multiple of p in [low, high]
            long long start = (low + p - 1) / p * p;
            if (start < p * p)
                start = p * p;

            for (long long j = start; j <= high; j += p)
                is_prime_local[j - low] = false;
        }
    }

    // Step 5: Count local primes
    long long local_count = 0;
    for (long long i = 0; i < size_local; i++)
        if (is_prime_local[i])
            local_count++;

    // Step 6: Reduce total count
    long long global_count = 0;
    MPI_Reduce(&local_count, &global_count, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();

    // Step 7: Print results
    if (rank == 0) {
        printf("Parallel Sieve of Eratosthenes Complete!\n");
        printf("Range: 2 to %lld\n", n);
        printf("Total primes found: %lld\n", global_count);
        printf("Elapsed time: %f seconds\n", end_time - start_time);
    }

    // Cleanup
    free(is_prime_local);
    free(base_primes);
    MPI_Finalize();
    return 0;
}
