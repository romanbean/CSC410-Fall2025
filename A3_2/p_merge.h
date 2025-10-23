#ifndef PARALLEL_SORTS_H
#define PARALLEL_SORTS_H

#define SIZE 100000
#define MAX_VAL 100000  // max random number

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    int* arr;
    int left;
    int right;
    int level;
} thread_data_t;

void merge(int* arr, int l, int m, int r);
void merge_sort_seq(int* arr, int l, int r);

void merge_sort_p(int* arr, int l, int r);
void* parallel_merge_sort(void* arg);

void printArray(int* arr, int n);
double time_diff_ms(struct timespec start, struct timespec end);

#endif