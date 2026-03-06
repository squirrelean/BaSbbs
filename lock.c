#include <pthread.h>

#include "lock.h"

static Lock rwlock = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0};

void read_lock()
{
    pthread_mutex_lock(&rwlock.mutex);

    while (rwlock.active_writer)
        pthread_cond_wait(&rwlock.read_allowed, &rwlock.mutex);

    rwlock.readers_num++;
    pthread_mutex_unlock(&rwlock.mutex);
}

void read_unlock()
{
    pthread_mutex_lock(&rwlock.mutex);
    rwlock.readers_num--;

    if (rwlock.readers_num == 0)
        pthread_cond_signal(&rwlock.write_allowed);

    pthread_mutex_unlock(&rwlock.mutex);
}

void write_lock()
{
    pthread_mutex_lock(&rwlock.mutex);

    while (rwlock.active_writer || rwlock.readers_num > 0)
        pthread_cond_wait(&rwlock.write_allowed, &rwlock.mutex);

    rwlock.active_writer = 1;
    pthread_mutex_unlock((&rwlock.mutex));
}

void write_unlock()
{
    pthread_mutex_lock(&rwlock.mutex);

    rwlock.active_writer = 0;

    pthread_cond_broadcast(&rwlock.read_allowed);
    pthread_cond_signal(&rwlock.write_allowed);

    pthread_mutex_unlock(&rwlock.mutex);
}
