#pragma once

#include <kernel/sched/types.h>

#define STACK_SIZE 4096

void thread_wait();
void thread_signal(thread_t*);

void thread_exit();
void thread_destroy(thread_t*);
void thread_cleanup(thread_t*);
void thread_priority(thread_t*, priority_t);

thread_t *thread_create(const char*, void*, void*, void*, void*);
thread_t *thread_handle();

void thread_idle_cleaning();
