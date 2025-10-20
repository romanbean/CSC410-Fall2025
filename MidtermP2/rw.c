// Implement your own read write lock using mutex and condition variables!

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t readers_ok;
    pthread_cond_t writers_ok;
    int readers;        // active readers
    int writers;        // active writers 
    int waiting_writers; // waiting writers
} rwlock_t;

rwlock_t lock;
int shared_data = 0;

// --- Custom Reader-Writer Lock Functions ---
void rwlock_init(rwlock_t *lock) 
{
    pthread_mutex_init(&lock->mutex, NULL);
    pthread_cond_init(&lock->readers_ok, NULL);
    pthread_cond_init(&lock->writers_ok, NULL);
    lock->readers = 0;
    lock->writers = 0;
    lock->waiting_writers = 0;
}

void rwlock_acquire_read(rwlock_t *lock) 
{
    pthread_mutex_lock(&lock->mutex);

    // Wait if there's an active writer or waiting writers (give priority)
    while (lock->writers > 0 || lock->waiting_writers > 0)
        pthread_cond_wait(&lock->readers_ok, &lock->mutex);

    lock->readers++;

    pthread_mutex_unlock(&lock->mutex);
}

void rwlock_release_read(rwlock_t *lock) 
{
    pthread_mutex_lock(&lock->mutex);

    lock->readers--;
    if (lock->readers == 0)
        pthread_cond_signal(&lock->writers_ok); // Wake up writers

    pthread_mutex_unlock(&lock->mutex);
}

void rwlock_acquire_write(rwlock_t *lock) 
{
    pthread_mutex_lock(&lock->mutex);
    lock->waiting_writers++;

    while (lock->readers > 0 || lock->writers > 0)
        pthread_cond_wait(&lock->writers_ok, &lock->mutex);

    lock->waiting_writers--;
    lock->writers = 1;

    pthread_mutex_unlock(&lock->mutex);
}

void rwlock_release_write(rwlock_t *lock) 
{
    pthread_mutex_lock(&lock->mutex);
    lock->writers = 0;

    // Prefer writers if any are waiting, else allow readers
    if (lock->waiting_writers > 0)
        pthread_cond_signal(&lock->writers_ok);
    else
        pthread_cond_broadcast(&lock->readers_ok);

    pthread_mutex_unlock(&lock->mutex);
}

// ################################################################################################
// ########## Provided to test your read write lock implementation ################################

void* reader(void* arg) 
{
    int id = *(int*)arg;
    for (int i = 0; i < 5; i++) 
    {
        rwlock_acquire_read(&lock);
        printf("Reader %d reads: %d\n", id, shared_data);
        usleep(100000);
        rwlock_release_read(&lock);
        usleep(100000);
    }
    return NULL;
}

void* writer(void* arg) 
{
    int id = *(int*)arg;
    for (int i = 0; i < 5; i++) 
    {
        rwlock_acquire_write(&lock);
        shared_data += 10;
        printf("Writer %d updates value to: %d\n", id, shared_data);
        usleep(150000); 
        rwlock_release_write(&lock);
        usleep(100000); 
    }
    return NULL;
}

int main() 
{
    pthread_t readers[5], writers[2];
    int r_ids[5], w_ids[2];

    rwlock_init(&lock);

    // reader threads
    for (int i = 0; i < 5; i++) 
    {
        r_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader, &r_ids[i]);
    }

    // writer threads
    for (int i = 0; i < 2; i++) 
    {
        w_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer, &w_ids[i]);
    }

    for (int i = 0; i < 5; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < 2; i++)
        pthread_join(writers[i], NULL);

    return 0;
}
