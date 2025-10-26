#pragma once

#include <kernel/lists.h>

typedef struct {
    spinlock_t lock;
    uint32_t free;
    list_t list;
} mutex_t;

int free_mutex(mutex_t *mutex);
mutex_t *create_mutex();
bool acquire_mutex(mutex_t *mutex, bool nonblock);
void release_mutex(mutex_t *mutex);
