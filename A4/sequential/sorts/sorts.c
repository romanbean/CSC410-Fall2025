#include "sorts.h"

// Merge Sort 
static void merge(int arr[], int left, int mid, int right) 
{
	int n1 = mid - left + 1; // length of left subarray
	int n2 = right - mid; // length of right subarray

	// Temp arrays
	int L[n1], R[n2];

	// Copying data to temp arrays
	for (int i = 0; i < n1; i++)
		L[i] = arr[left + i];
	for (int j = 0; j < n2; j++)
		R[j] = arr[mid + 1 + j];

	// Merge temp arrays back into arr[left..right]
	int i = 0, j = 0, k = left;
	while (i < n1 && j < n2) {
		if (L[i] <= R[j]) {
			arr[k] = L[i];
			i++;
		} else {
			arr[k] = R[j];
			j++;
		}
		k++;
	}

	// Copy any remaining elements of L[]
	while (i < n1) {
		arr[k] = L[i];
		i++;
		k++;
	}

	// Copy any remaining elements of R[]
	while (j < n2) {
		arr[k] = R[j];
		j++;
		k++;
	}

}

void mergeSort(int arr[], int left, int right) 
{
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

// Bubble Sort
void bubbleSort(int arr[], int n) 
{
	for (int i = 0; i < n-1; i++) {
		// last i elements are already in palce
		for (int j = 0; j < n-1; j++) {
			if (arr[j] > arr[j + 1]) {
				// swap arr[j] and arr[j+1]
				int temp = arr[j];
				arr[j] = arr[j+1];
				arr[j + 1] = temp;
			}
		}
	}
}
void printArray(int arr[], int n) 
{
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");
}
