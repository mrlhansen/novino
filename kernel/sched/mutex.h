#pragma once

#include <kernel/sched/wq.h>

typedef struct {
    spinlock_t lock;
    wq_t queue;
    bool free;
} mutex_t;

int free_mutex(mutex_t *mutex);
mutex_t *create_mutex();
bool acquire_mutex(mutex_t *mutex, bool nonblock);
void release_mutex(mutex_t *mutex);
