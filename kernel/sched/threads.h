#pragma once

#include <kernel/sched/types.h>

#define STACK_SIZE 4096

void thread_wait(); // block?
void thread_signal(thread_t *thread); // unblock?
void thread_sleep(size_t ns);

void thread_exit();
void thread_priority(thread_t *thread, priority_t priority);

thread_t *thread_create(const char*, void*, void*, void*, void*);
thread_t *thread_handle();

void thread_idle_cleaning();
