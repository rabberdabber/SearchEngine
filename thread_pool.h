#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define NUM_OF_THREADS 10



typedef struct job
{
    void (*exec_func)(void * data);
    void *func_data;
} job_t;

typedef struct worker_thread
{
    pthread_t tid;
} worker_thread_t;

int init_pool();

void destroy_pool();

void add_job_to_pool(job_t *job);

#endif
