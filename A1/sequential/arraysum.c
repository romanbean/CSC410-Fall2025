#include <stdio.h>

// test size
// #define SIZE 10

// default size 10,000
// #define SIZE 10000

// user determined size 100,000
#define SIZE 100000

int sumArray(int arr[], int size) 
{
	int total=0;
 	for(int i = 0; i < size; i++) {
		total += arr[i];
	}
	return total;
}

int main() 
{
    int arr[SIZE];
    for (int i = 0; i < SIZE; i++) {
        arr[i] = i + 1; 
    }

    int totalSum = sumArray(arr, SIZE);
    printf("Total Sum: %d\n", totalSum);

    return 0;
}
