#include <kernel/time/timer.h>
#include <kernel/time/time.h>
#include <kernel/errno.h>

LIST_INIT(timers, timer_t, link);

static uint64_t next_deadline = ~0UL;
static spinlock_t lock;

void timer_tick()
{
    timer_t *item, *next;
    uint32_t flags;
    uint64_t ts;
    int64_t dt;

    // We assume that the timer ticks at 1000 Hz, which means that we can make the callback
    // with an accurary of 500us, assuming the time source is more accurate than the timer source.
    // When we are more than 500us away from the deadline, we postpone the callback to the next tick.

    if(!next_deadline)
    {
        return;
    }

    ts = system_timestamp();
    dt = ts - next_deadline + NANOSECONDS(500, TIME_US);
    if(dt < 0)
    {
        return;
    }

    acquire_safe_lock(&lock, &flags);

    while(item = list_head(&timers), item)
    {
        dt = ts - item->deadline + NANOSECONDS(500, TIME_US);
        if(dt < 0)
        {
            break;
        }

        next = item->link.next;
        list_pop(&timers);
        item->callback(item);

        if(next)
        {
            next_deadline = next->deadline;
        }
        else
        {
            next_deadline = 0;
        }
    }

    release_safe_lock(&lock, &flags);
}

int timer_start(timer_t *tm)
{
    timer_t *item = 0;
    uint32_t flags;
    uint64_t ts;

    if(!tm->callback)
    {
        return -EINVAL;
    }

    acquire_safe_lock(&lock, &flags);

    ts = system_timestamp();
    tm->deadline = ts + tm->period;

    if(tm->deadline < next_deadline)
    {
        next_deadline = tm->deadline;
    }
    else if(!next_deadline)
    {
        next_deadline = tm->deadline;
    }

    while(item = list_iterate(&timers, item), item)
    {
        if(item->deadline > tm->deadline)
        {
            break;
        }
    }

    if(!item)
    {
        list_append(&timers, tm);
    }
    else
    {
        list_insert_before(&timers, item, tm);
    }

    release_safe_lock(&lock, &flags);
    return 0;
}

void timer_cancel(timer_t *tm)
{
    uint32_t flags;

    acquire_safe_lock(&lock, &flags);

    list_remove(&timers, tm);
    tm = list_head(&timers);

    if(tm)
    {
        next_deadline = tm->deadline;
    }
    else
    {
        next_deadline = 0;
    }

    release_safe_lock(&lock, &flags);
}
