// Sequential sums for reference

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int *arr = malloc(N * sizeof(int));
    if (!arr) {
        perror("malloc");
        return 1;
    }

    for (int i = 0; i < N; i++) {
        arr[i] = i + 1;
    }

    long long total = 0;
    for (int i = 0; i < N; i++) {
        total += arr[i];
    }

    printf("Total sum = %lld\n", total);
    free(arr);
    
    return 0;
}
