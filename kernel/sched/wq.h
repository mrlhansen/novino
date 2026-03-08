#pragma once

#include <kernel/sched/types.h>

void wq_init(wq_t *wq);
void wq_lock(wq_t *wq);
void wq_unlock(wq_t *wq);

int wq_wait(wq_t *wq);
int wq_wake(wq_t *wq);
int wq_wake_one(wq_t *wq);

int wq_size(wq_t *wq);

void wq_interrupt(thread_t *thread);
