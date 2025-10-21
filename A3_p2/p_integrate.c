#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N 1000000000 // intervals
#define NUM_THREADS 1

double f(double x) {
    return 4.0 / (1.0 + x * x);
}

// use this shared global variable
double total_sum = 0.0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int thread_id;
    long start;
    long end;
    double step;
} thread_data_t;

void *parallel_trapezoidalRule(void *arg) 
{
    thread_data_t *data = (thread_data_t *)arg;
    long start = data->start;
    long end = data->end;
    double step = data->step;

    double local_sum = 0.0;
    for (long i = start; i < end; i++) {
        double x1 = i * step;
        double x2 = (i + 1) * step;
        local_sum += 0.5 * (f(x1) + f(x2)) * step;
    }

    pthread_mutex_lock(&mutex);
    total_sum += local_sum;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) 
{
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    double step = 1.0 / (double)N;
    long chunk = N / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].step = step;
        thread_data[i].start = i * chunk;
        // Ensure last thread covers up to N
        thread_data[i].end = (i == NUM_THREADS - 1) ? N : (i + 1) * chunk;

        int rc = pthread_create(&threads[i], NULL, parallel_trapezoidalRule, &thread_data[i]);
        if (rc) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(-1);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Result of numerical integration: %f\n", total_sum);

    return 0;
}
