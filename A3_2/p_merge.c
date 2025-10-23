#include "p_merge.h"
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 4

void merge(int* arr, int l, int m, int r) 
{
    int n1 = m - l + 1;
    int n2 = r - m;

    int* L = (int*)malloc(n1 * sizeof(int));
    int* R = (int*)malloc(n2 * sizeof(int));

    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j])
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}

void merge_sort_seq(int* arr, int l, int r) {
    if (l < r) {
        int m = (l + r) / 2;
        merge_sort_seq(arr, l, m);
        merge_sort_seq(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

// ------------- Approach 1: Segment-based ---------------------

typedef struct {
    int* arr;
    int left;
    int right;
} segment_data_t;

void* segment_sort(void* arg) {
    segment_data_t* data = (segment_data_t*)arg;
    merge_sort_seq(data->arr, data->left, data->right);
    pthread_exit(NULL);
}

void merge_sort_p_segment(int* arr, int l, int r) {
    int num_threads = NUM_THREADS;
    pthread_t threads[num_threads];
    segment_data_t thread_data[num_threads];

    int segment_size = (r - l + 1) / num_threads;

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].arr = arr;
        thread_data[i].left = l + i * segment_size;
        thread_data[i].right = (i == num_threads - 1) ? r : (thread_data[i].left + segment_size - 1);
        pthread_create(&threads[i], NULL, segment_sort, &thread_data[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Merge all sorted segments
    int merged_left = thread_data[0].left;
    int merged_right = thread_data[0].right;

    for (int i = 1; i < num_threads; i++) {
        merge(arr, merged_left, merged_right, thread_data[i].right);
        merged_right = thread_data[i].right;
    }
}

// ------------- Approach 2: Recursive-thread-creation -----------

pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
int current_thread_count = 1;
int max_threads = NUM_THREADS;

typedef struct {
    int* arr;
    int left;
    int right;
} recursive_data_t;

void* recursive_parallel_merge_sort(void* arg) {
    recursive_data_t* data = (recursive_data_t*)arg;
    int l = data->left;
    int r = data->right;

    if(l >= r) pthread_exit(NULL);

    int m = (l + r) / 2;

    int spawn_left = 0, spawn_right = 0;
    pthread_t left_thread, right_thread;

    pthread_mutex_lock(&thread_count_mutex);
    if(current_thread_count < max_threads) {
        current_thread_count++;
        spawn_left = 1;
    }
    if(current_thread_count < max_threads) {
        current_thread_count++;
        spawn_right = 1;
    }
    pthread_mutex_unlock(&thread_count_mutex);

    recursive_data_t left_data = {data->arr, l, m};
    recursive_data_t right_data = {data->arr, m + 1, r};

    if(spawn_left) pthread_create(&left_thread, NULL, recursive_parallel_merge_sort, &left_data);
    else merge_sort_seq(data->arr, l, m);

    if(spawn_right) pthread_create(&right_thread, NULL, recursive_parallel_merge_sort, &right_data);
    else merge_sort_seq(data->arr, m + 1, r);

    if(spawn_left) pthread_join(left_thread, NULL);
    if(spawn_right) pthread_join(right_thread, NULL);

    merge(data->arr, l, m, r);

    pthread_mutex_lock(&thread_count_mutex);
    if(spawn_left) current_thread_count--;
    if(spawn_right) current_thread_count--;
    pthread_mutex_unlock(&thread_count_mutex);

    pthread_exit(NULL);
}

void merge_sort_p_recursive(int* arr, int l, int r) {
    recursive_data_t data = {arr, l, r};
    recursive_parallel_merge_sort(&data);
}

// ------------- Approach 3: Thread Pool (Outline) ------------

typedef enum { TASK_SORT, TASK_MERGE } task_type_t;

typedef struct task {
    task_type_t type;
    int left, mid, right;
    int* arr;
    struct task* next;
} task_t;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
task_t* task_queue = NULL;
int tasks_in_progress = 0;

void enqueue_task(task_t* task) {
    pthread_mutex_lock(&queue_mutex);
    // add task to queue tail
    task->next = NULL;
    if (!task_queue) task_queue = task;
    else {
        task_t* last = task_queue;
        while (last->next) last = last->next;
        last->next = task;
    }
    tasks_in_progress++;
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

task_t* dequeue_task() {
    pthread_mutex_lock(&queue_mutex);
    while (!task_queue) pthread_cond_wait(&queue_cond, &queue_mutex);
    task_t* task = task_queue;
    task_queue = task_queue->next;
    pthread_mutex_unlock(&queue_mutex);
    return task;
}

void* thread_pool_worker(void* arg) {
    while (1) {
        task_t* task = NULL;
        pthread_mutex_lock(&queue_mutex);
        while (!task_queue) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        task = task_queue;
        task_queue = task_queue->next;
        pthread_mutex_unlock(&queue_mutex);

        if (task == NULL) break;

        if (task->type == TASK_SORT) {
            merge_sort_seq(task->arr, task->left, task->right);
        } else if (task->type == TASK_MERGE) {
            merge(task->arr, task->left, task->mid, task->right);
        }

        free(task);

        pthread_mutex_lock(&queue_mutex);
        tasks_in_progress--;
        if (tasks_in_progress == 0) {
            pthread_cond_signal(&queue_cond);
        }
        pthread_mutex_unlock(&queue_mutex);
    }
    pthread_exit(NULL);
}

pthread_t thread_pool[NUM_THREADS];

void merge_sort_p_threadpool(int* arr, int l, int r) {
    // Initialize thread pool
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&thread_pool[i], NULL, thread_pool_worker, NULL);
    }

    // For simplicity, divide into NUM_THREADS segments and enqueue sort tasks
    int segment_size = (r - l + 1) / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; i++) {
        task_t* task = (task_t*)malloc(sizeof(task_t));
        task->type = TASK_SORT;
        task->arr = arr;
        task->left = l + i * segment_size;
        task->right = (i == NUM_THREADS - 1) ? r : (task->left + segment_size - 1);
        enqueue_task(task);
    }

    // Wait until all sorting tasks finished
    pthread_mutex_lock(&queue_mutex);
    while (tasks_in_progress > 0) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);

    // Merge segments sequentially for now
    int merged_left = l;
    int merged_right = l + segment_size - 1;
    for (int i = 1; i < NUM_THREADS; i++) {
        int right_end = (i == NUM_THREADS - 1) ? r : (l + (i + 1)*segment_size - 1);
        merge(arr, merged_left, merged_right, right_end);
        merged_right = right_end;
    }

    // Shutdown thread pool by sending NULL tasks
    for (int i = 0; i < NUM_THREADS; i++) {
        enqueue_task(NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread_pool[i], NULL);
    }
}

// ------------- merge_sort_p selects approach --------------------

void merge_sort_p(int* arr, int l, int r)
{
    // Approach 1: Segment-based
//    merge_sort_p_segment(arr, l, r);

    // Approach 2: Recursive thread creation
//    merge_sort_p_recursive(arr, l, r);

    // Approach 3: Thread pool
    merge_sort_p_threadpool(arr, l, r);
}

void printArray(int* arr, int n) {
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");
   // }
}

double time_diff_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1000000.0;
}
