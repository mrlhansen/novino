#pragma once

#include <kernel/syscalls/syscalls.h>
#include <kernel/sched/process.h>
#include <kernel/x86/tss.h>
#include <kernel/lists.h>

typedef struct {
    uint64_t rsp;             // Address for the scheduling function stack (must be struct offset 0)
    uint64_t xstate;          // Address for extended state context (must be struct offset 8)

    uint32_t apic_id;         // Local APIC ID
    uint64_t frequency;       // Local APIC counter frequency

    uint64_t tsc0;            // Used for calculating elapsed time
    uint64_t tsc1;            // Used for calculating current load
    uint64_t tsc2;            // Used for calculating current load
    uint32_t load;            // Current CPU core load in permille

    list_t idle;              // Queue for policy 2
    list_t srt;               // Queue for policy 0
    list_t std[9];            // Queue for policy 1
    int index;                // Index into policy 1 queue
    int count;                // Number of threads in policy 1 queue

    thread_t *thread_current; // Currently executed thread
    thread_t *thread_idle;    // Idle thread for this core
    spinlock_t lock;          // Global lock for accessing this struct
    tss_t *tss;               // TSS descripter for this core
} scheduler_t;

void switch_task();
thread_t *scheduler_get_thread();

void scheduler_mask();
void scheduler_unmask();
void scheduler_stop();
void scheduler_yield();

void scheduler_append(thread_t*);
void scheduler_init_core(int, int, int, tss_t*);
void scheduler_init();
