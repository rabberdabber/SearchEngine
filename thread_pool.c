#include "thread_pool.h"
#include "tc_malloc.h"
#include <stdio.h>


pthread_cond_t _cond_mutex = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex_lock = PTHREAD_MUTEX_INITIALIZER;

struct Job_Queue * Job_queue = NULL;


struct Job_Node {
    job_t * job;
    struct Job_Node * next;
};

struct Job_Queue {
    int size;
    struct Job_Node * head;
    struct Job_Node * tail;
};

void create_job_queue()
{
    Job_queue = (struct Job_Queue *) tc_calloc(1,sizeof(struct Job_Queue));
    if(!Job_queue)
    {
        fprintf(stderr, "tc_calloc failed\n");
        return;
    }

    Job_queue->head = Job_queue->tail = NULL;

}

int enqueue_job(struct Job_Node * job_node)
{
    if(Job_queue->head == NULL)
    {
       Job_queue->tail =  Job_queue->head = job_node;
    }

    else
    {
        Job_queue->tail->next = job_node;
        Job_queue->tail = job_node;
    }

    Job_queue->size++;
    return 1;
}

job_t * dequeue_job()
{
    if(Job_queue->size == 0)
    {
        return NULL;
    }

    struct Job_Node * tmp = Job_queue->head;
    job_t * job = tmp->job;
    Job_queue->head = tmp->next;

    Job_queue->size--;
    
    return job;
}


static void *
activate_workers()
{
    job_t *job = NULL;

    void *threadcache;

    if ((threadcache = tc_thread_init()) == NULL)
    {
        fprintf(stderr, "thread local initialization failed\n");
        return NULL;
    }

    for (;;)
    {

        pthread_mutex_lock(&_mutex_lock);

        // wait till job_queue is non-empty
        while (Job_queue->size == 0)
        {
            pthread_cond_wait(&_cond_mutex, &_mutex_lock);
        }

        if (Job_queue->size)
        {
            job = dequeue_job();
        }

        pthread_mutex_unlock(&_mutex_lock);

        // nothing to do
        if (!job)
        {
            continue;
        }

        job->exec_func(job->func_data);
    }

    return NULL;
}

int init_pool()
{
    int i;
    worker_thread_t *tmp;
    create_job_queue();

    for (i = 0; i < NUM_OF_THREADS; i++)
    {
        tmp = (worker_thread_t *)tc_calloc(1, sizeof(worker_thread_t));

        if (!tmp)
        {
            fprintf(stderr, "tc_calloc failed\n");
            return 0;
        }

        int ret = pthread_create(&tmp->tid, NULL,activate_workers,NULL);
        if (ret != 0)
        {
            perror("pthread_create");
            return 0;
        }
        else
        {
            printf("created thread %d for threadpool\n",i+1);
            fflush(stdout);
        }
        
    }

    return 1;
}

void add_job_to_pool(job_t *job)
{
    pthread_mutex_lock(&_mutex_lock);

    struct Job_Node * job_node = (struct Job_Node *)tc_calloc(1,sizeof(struct Job_Node));

    if (!job_node)
    {
        fprintf(stderr, "tc_calloc failed\n");
        return ;
    }

    job_node->job = job;
    enqueue_job(job_node);
    
    // notify workers
    pthread_cond_signal(&_cond_mutex);
    pthread_mutex_unlock(&_mutex_lock);
}
