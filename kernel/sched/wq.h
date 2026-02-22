#pragma once

#include <kernel/atomic.h>
#include <kernel/lists.h>

typedef struct {
    spinlock_t lock;   // Spinlock
    uint32_t count;    // Lock counter
    uint32_t flags;    // Interrupt flags
    void *owner;       // Thread holding the lock
    list_t list;       // List of threads
} wq_t;

void wq_init(wq_t *wq);
void wq_lock(wq_t *wq);
void wq_unlock(wq_t *wq);

int wq_wait(wq_t *wq);
int wq_wake(wq_t *wq);
int wq_wake_one(wq_t *wq);

int wq_size(wq_t *wq);
