#pragma once

#include <kernel/sched/threads.h>

void kthreads_run(thread_t*);
thread_t *kthreads_create(const char*, void*, void*, priority_t);
thread_t *kthreads_create_idle();
void kthreads_init();
