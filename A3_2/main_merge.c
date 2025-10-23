#include "p_merge.h"

int main() 
{
    struct timespec start, end;

    int* arr1 = (int*)malloc(SIZE * sizeof(int));
    int* arr2 = (int*)malloc(SIZE * sizeof(int));

    printf("Enter elements: ");

    for (int i = 0; i < SIZE; i++) 
    {
        int val = rand() % MAX_VAL;
        arr1[i] = val;
        arr2[i] = val;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    merge_sort_seq(arr1, 0, SIZE - 1);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Sequential Merge Sort Time: %.3f ms\n", time_diff_ms(start, end));
    printArray(arr1, SIZE);


    clock_gettime(CLOCK_MONOTONIC, &start);
    merge_sort_p(arr2, 0, SIZE - 1);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Parallel Merge Sort Time: %.3f ms\n", time_diff_ms(start, end));
    printArray(arr2, SIZE);

    free(arr1);
    free(arr2);

    return 0;
}
