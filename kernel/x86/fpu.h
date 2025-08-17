#pragma once

#include <kernel/sched/threads.h>

extern int xsave_support;

int fpu_xstate_size();
void fpu_xstate_init(thread_t *thread);
void fpu_init();
