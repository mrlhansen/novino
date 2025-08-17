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

void acquire_mutex(mutex_t *mutex)
{
    thread_t *thread;
    int yield;

    scheduler_mask();
    acquire_lock(&mutex->lock);
    thread = thread_handle();

    if(mutex->free)
    {
        mutex->free = 0;
        yield = 0;
    }
    else
    {
        yield = 1;
        thread->state = WAITING;
        list_append(&mutex->list, thread);
        list_append(&waiting, thread);
    }

    release_lock(&mutex->lock);
    scheduler_unmask();

    if(yield)
    {
        scheduler_yield();
    }
}

void release_mutex(mutex_t *mutex)
{
    thread_t *thread;

    scheduler_mask();
    acquire_lock(&mutex->lock);

    if(mutex->list.length == 0)
    {
        mutex->free = 1;
    }
    else
    {
        thread = list_pop(&mutex->list);
        if(list_remove(&waiting, thread) == 0)
        {
            thread->state = READY;
            scheduler_append(thread);
        }
    }

    release_lock(&mutex->lock);
    scheduler_unmask();
}
