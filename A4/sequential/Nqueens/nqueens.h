#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <omp.h>

bool isSafe(int board[], int row, int col, int n) 
{
    for (int i = 0; i < col; i++) {
        if (board[i] == row || abs(board[i] - row) == abs(i - col)) {
            return false;
        }
    }
    return true;
}

void printBoard(int board[], int n) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (board[j] == i) {
				printf("Q ");
			} else {
				printf(". ");
			}
		}
		printf("\n");
	}
	printf("\n");
}

// Wrapper for parallel execution at first level
void solveNQueensUtil(int board[], int col, int n) {
    if (col == n) {
        printBoard(board, n);
        return;
    }

    if (col == 0) {
        #pragma omp parallel num_threads(1)
        {
            #pragma omp single
            {
                for (int i = 0; i < n; i++) {
                    if (isSafe(board, i, col, n)) {
                        int* newBoard = (int*)malloc(n * sizeof(int));
                        for (int k = 0; k < n; k++) newBoard[k] = board[k];
                        newBoard[col] = i;
                        #pragma omp task firstprivate(newBoard)
                        solveNQueensUtil(newBoard, col + 1, n);
                        #pragma omp taskwait
                        free(newBoard);
                    }
                }
            }
        }
    } else {
        // Sequential recursion for deeper columns
        for (int i = 0; i < n; i++) {
            if (isSafe(board, i, col, n)) {
                board[col] = i;
                solveNQueensUtil(board, col + 1, n);
            }
        }
    }
}

