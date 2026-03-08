#include <kernel/sched/threads.h>
#include <kernel/sched/mutex.h>
#include <kernel/sched/wq.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <kernel/lists.h>

mutex_t *create_mutex()
{
    mutex_t *mutex;

    mutex = kzalloc(sizeof(mutex_t));
    if(!mutex)
    {
        return 0;
    }

    wq_init(&mutex->queue);
    mutex->lock = 0;
    mutex->free = true;

    return mutex;
}

int free_mutex(mutex_t *mutex)
{
    if(!mutex->free)
    {
        return -EBUSY;
    }
    kfree(mutex);
    return 0;
}

bool acquire_mutex(mutex_t *mutex, bool nonblock)
{
    thread_t *thread;
    bool status;

    thread = thread_handle();
    status = true;

    wq_lock(&mutex->queue);

    if(mutex->free)
    {
        mutex->free = false;
        thread->yield = 0;
    }
    else
    {
        if(nonblock)
        {
            status = false;
            thread->yield = 0;
        }
        else
        {
            thread->yield = 1;
        }
    }

    if(thread->yield)
    {
        wq_wait(&mutex->queue);
    }
    else
    {
        wq_unlock(&mutex->queue);
    }

    return status;
}

void release_mutex(mutex_t *mutex)
{
    int status;

    wq_lock(&mutex->queue);

    status = wq_wake_one(&mutex->queue);
    if(!status)
    {
        mutex->free = true;
    }

    wq_unlock(&mutex->queue);
}
