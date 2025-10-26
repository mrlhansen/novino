#include <kernel/sched/scheduler.h>
#include <kernel/sched/mutex.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <kernel/lists.h>

static LIST_INIT(waiting, thread_t, slink);

mutex_t *create_mutex()
{
    mutex_t *mutex;

    mutex = kzalloc(sizeof(mutex_t));
    if(mutex == 0)
    {
        return 0;
    }

    list_init(&mutex->list, offsetof(thread_t, mlink));
    mutex->lock = 0;
    mutex->free = 1;

    return mutex;
}

int free_mutex(mutex_t *mutex)
{
    if(mutex->free == 0)
    {
        return -EBUSY;
    }

    kfree(mutex);
    return 0;
}

bool acquire_mutex(mutex_t *mutex, bool nonblock)
{
    thread_t *thread;
    uint32_t flags;
    bool status;

    thread = thread_handle();
    status = true;

    acquire_safe_lock(&mutex->lock, &flags);

    if(mutex->free)
    {
        mutex->free = 0;
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
            thread->state = WAITING;
            list_append(&mutex->list, thread);
            list_append(&waiting, thread);
        }
    }

    release_lock(&mutex->lock);

    if(thread->yield)
    {
        scheduler_yield();
    }

    restore_interrupts(&flags);
    return status;
}

void release_mutex(mutex_t *mutex)
{
    thread_t *thread;

    scheduler_mask();
    acquire_lock(&mutex->lock);
    thread = list_pop(&mutex->list);

    if(thread)
    {
        if(list_remove(&waiting, thread) == 0)
        {
            thread->state = READY;
            scheduler_append(thread);
        }
    }
    else
    {
        mutex->free = 1;
    }

    release_lock(&mutex->lock);
    scheduler_unmask();
}
