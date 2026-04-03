#include <kernel/sched/scheduler.h>
#include <kernel/sched/threads.h>
#include <kernel/sched/wq.h>
#include <kernel/errno.h>

void wq_init(wq_t *wq)
{
    wq->lock = 0;
    wq->count = 0;
    wq->flags = 0;
    wq->owner = 0;
    list_init(&wq->list, offsetof(thread_t, wlink));
}

void wq_lock(wq_t *wq)
{
    thread_t *self;
    uint32_t flags;

    self = thread_handle();
    if(wq->owner == self)
    {
        wq->count++;
        return;
    }

    acquire_safe_lock(&wq->lock, &flags);
    wq->flags = flags;
    wq->owner = self;
    wq->count = 1;
}

void wq_unlock(wq_t *wq)
{
    wq->count--;
    if(!wq->count)
    {
        wq->owner = 0;
        release_safe_lock(&wq->lock, &wq->flags);
    }
}

int wq_wait(wq_t *wq)
{
    thread_t *self;
    uint32_t flags;

    scheduler_stop();
    wq_lock(wq);

    flags = wq->flags;
    self = wq->owner;

    wq->owner = 0;
    wq->count = 0;

    self->wq = wq;
    self->lock = &wq->lock;
    self->state = SLEEPING;

    list_append(&wq->list, self);
    scheduler_yield();
    restore_interrupts(&flags);

    if(self->signals & SIGINTR)
    {
        self->signals &= ~SIGINTR;
        return -EINTR;
    }

    return 0;
}

int wq_wake(wq_t *wq)
{
    thread_t *item;
    int count = 0;

    wq_lock(wq);

    while(item = list_pop(&wq->list), item)
    {
        item->state = READY;
        item->wq = 0;
        scheduler_append(item);
        count++;
    }

    wq_unlock(wq);
    return count;
}

int wq_wake_one(wq_t *wq)
{
    thread_t *item;
    int count = 0;

    wq_lock(wq);

    item = list_pop(&wq->list);
    if(item)
    {
        item->state = READY;
        item->wq = 0;
        scheduler_append(item);
        count++;
    }

    wq_unlock(wq);
    return count;
}

int wq_size(wq_t *wq)
{
    return wq->list.length;
}

void wq_interrupt(thread_t *thread)
{
    wq_t *wq;
    bool ok;

    wq = thread->wq;
    if(!wq)
    {
        return;
    }

    wq_lock(wq);

    ok = list_remove(&wq->list, thread);
    if(ok)
    {
        thread->signals |= SIGINTR;
        thread->state = READY;
        thread->wq = 0;
        scheduler_append(thread);
    }

    wq_unlock(wq);
}
