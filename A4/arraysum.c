#include <stdio.h>
#include <omp.h>

#define SIZE 100000

long long int sumArray(long long int arr[], int size) 
{
    long long int total = 0;

    // Use OpenMP parallel for with reduction
    #pragma omp parallel for reduction(+:total) num_threads(9)
    for (int i = 0; i < size; i++) {
        total += arr[i];
    }

    return total;
}

int main() 
{
    long long int arr[SIZE];
    
    for (int i = 0; i < SIZE; i++) {
        arr[i] = i + 1;
    }

    double start_time = omp_get_wtime();
    long long int totalSum = sumArray(arr, SIZE);
    double end_time = omp_get_wtime();

    printf("Total Sum: %lld\n", totalSum);
    printf("Execution Time: %f seconds\n", end_time - start_time);

    return 0;
}
