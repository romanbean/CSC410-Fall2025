#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N 1000 // Adjust for testing

void displayMatrix(int* matrix, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j)
            printf("%d ", matrix[i * n + j]);
        printf("\n");
    }
}

void matrixMultiply(int** A, int** B, int** C, int n) 
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			C[i][j] = 0;
			for (int k = 0; k < n; k++)
			{
				C[i][j] += A[i][k] * B[k][j];
			}
		}
	}
}
