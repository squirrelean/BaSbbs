#ifndef LOCK_H
#define LOCK_H
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t read_allowed;
    pthread_cond_t write_allowed;
    int readers_num;
    int active_writer;
} Lock;

void read_lock();
void read_unlock();
void write_lock();
void write_unlock();

#endif
