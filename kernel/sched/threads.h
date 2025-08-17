#pragma once

#include <kernel/lists.h>

#define STACK_SIZE 4096
typedef struct thread thread_t;
typedef struct process process_t;
typedef long pid_t;
typedef volatile uint32_t yield_t;

typedef enum {
    READY,
    RUNNING,
    SLEEPING,
    BLOCKING,
    WAITING,
    TERMINATED
} state_t;

typedef enum {
    TPR_IDLE,
    TPR_LOW,
    TPR_MID,
    TPR_HIGH,
    TPR_SRT
} priority_t;

struct thread {
    char name[32];              // Thread name
    pid_t tid;                  // Thread ID
    state_t state;              // Current state
    priority_t priority;        // Scheduler priority
    uint8_t max_count;          // Maximal count for variable frequency scheduling
    uint64_t time_used;         // CPU time consumed
    uint64_t xstate;            // Address for extended state context (mainly FPU registers)
    uint64_t stack;             // Top address for kernel space stack
    uint64_t rsp;               // Current position in stack
    process_t *parent;          // Parent process
    link_t plink;               // Link for threads in parent process
    link_t slink;               // Link for threads in scheduler queue
    link_t mlink;               // Link for threads in mutex queue
    spinlock_t *lock;           // Optional lock used for safe thread blocking
    yield_t yield;              // Thread is currently yielding
    struct {                    // Used for IRQ signals
        int recv;               // Signal received
        int wait;               // Waiting for signal
        int core;               // Core ID of waiting thread
        spinlock_t lock;        // Synchronization lock
    } sig;
    struct {                    // Used in swapgs
        uint64_t krsp;          // Kernel RSP
        uint64_t ursp;          // User RSP
    } gs;
};

void thread_block(spinlock_t*);
void thread_unblock(thread_t*);

void thread_wait();
void thread_signal(thread_t*);

void thread_exit();
void thread_destroy(thread_t*);
void thread_cleanup(thread_t*);
void thread_priority(thread_t*, priority_t);

thread_t *thread_create(const char*, void*, void*, void*, void*);
thread_t *thread_handle();
