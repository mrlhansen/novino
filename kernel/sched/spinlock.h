#pragma once

#include <kernel/types.h>

typedef volatile uint32_t spinlock_t;

void disable_interrupts(uint32_t*);
void restore_interrupts(uint32_t*);

void acquire_safe_lock(spinlock_t*, uint32_t*);
void release_safe_lock(spinlock_t*, uint32_t*);

void acquire_lock(spinlock_t*);
void release_lock(spinlock_t*);
