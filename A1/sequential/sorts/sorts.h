#ifndef SORT_H
#define SORT_H

//#define SIZE 30
#define SIZE 10000
#define MAX_VAL 100000  // max random number

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void mergeSort(int arr[], int left, int right);
void bubbleSort(int arr[], int n);

void printArray(int arr[], int n);

#endif
