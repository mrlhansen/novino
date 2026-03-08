#pragma once

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/vfs/types.h>

typedef struct thread thread_t;
typedef struct process process_t;
typedef long pid_t;
typedef volatile uint32_t yield_t;

typedef enum {
    READY,
    RUNNING,
    SLEEPING,
    BLOCKING,
    TERMINATED
} state_t;

typedef enum {
    TPR_IDLE,
    TPR_LOW,
    TPR_MID,
    TPR_HIGH,
    TPR_SRT
} priority_t;

typedef struct {
    spinlock_t lock;   // Spinlock
    uint32_t count;    // Lock counter
    uint32_t flags;    // Interrupt flags
    thread_t *owner;   // Thread holding the lock
    list_t list;       // List of threads
} wq_t;

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
    link_t wlink;               // Link for threads in wait queue
    wq_t *wq;                   // Current wait queue
    spinlock_t *lock;           // Optional lock used for safe thread blocking
    yield_t yield;              // Thread is currently yielding
    bool terminate;             // Terminate thread on next scheduling loop
    struct {                    // Used for IRQ signals
        int recv;               // Signal received
        int wait;               // Waiting for signal
        spinlock_t lock;        // Synchronization lock
    } sig;
    struct {                    // Used in swapgs
        uint64_t krsp;          // Kernel RSP
        uint64_t ursp;          // User RSP
    } gs;
};

struct process {
    char name[32];        // Process name
    int exitcode;         // Exit code when process has terminated
    state_t state;        // Process state (running or terminated)
    pid_t pid;            // Process ID
    pid_t next_tid;       // Next thread ID
    uint64_t pml4;        // Physical address of PML4
    uint32_t uid;         // User ID
    uint32_t gid;         // Group ID
    dentry_t *cwd;        // Current working directory
    process_t *parent;    // Parent process
    list_t threads;       // List of threads
    list_t children;      // List of children
    link_t sibling;       // Link in child processes  -- clink
    link_t plink;         // Link in global processes -- glink
    list_t mmap;          // List of memory maps
    spinlock_t lock;      // Lock for this struct
    wq_t wait;            // List for threads in wait() calls
    struct {
        size_t next;      // Next file descriptor ID
        list_t list;      // List of open file descriptors
    } fd;
    struct {
        size_t start;     // Data segment start
        size_t end;       // Data segment end
        size_t max;       // Data segment max
    } brk;
};
