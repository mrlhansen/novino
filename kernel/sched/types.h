#pragma once

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/vfs/types.h>

typedef struct thread thread_t;
typedef struct process process_t;
typedef long pid_t;
typedef volatile uint32_t yield_t;

enum {
    SIGEXIT = (1 << 0), // Exit process
    SIGTERM = (1 << 1), // Terminate thread
    SIGINTR = (1 << 2), // Interrupted
};

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

typedef struct {
    size_t r15;    // x86-64 ABI: preserve
    size_t r14;    // x86-64 ABI: preserve
    size_t r13;    // x86-64 ABI: preserve
    size_t r12;    // x86-64 ABI: preserve
    size_t r11;    // x86-64 ABI: volatile
    size_t r10;    // x86-64 ABI: volatile
    size_t r9;     // x86-64 ABI: volatile (6th argument)
    size_t r8;     // x86-64 ABI: volatile (5th argument)
    size_t rbp;    // x86-64 ABI: preserve (frame pointer)
    size_t rsi;    // x86-64 ABI: volatile (2nd argument)
    size_t rdi;    // x86-64 ABI: volatile (1st argument)
    size_t rdx;    // x86-64 ABI: volatile (3rd argument)
    size_t rcx;    // x86-64 ABI: volatile (4th argument)
    size_t rbx;    // x86-64 ABI: preserve
    size_t rax;    // x86-64 ABI: volatile (return value)
    size_t rip;    // instruction pointer
    size_t cs;     // code segment
    size_t rflags; // status register
    size_t rsp;    // stack pointer
    size_t ss;     // stack segment
} stack_t;

struct thread {
    char name[32];              // Thread name
    pid_t tid;                  // Thread ID
    state_t state;              // Current state
    priority_t priority;        // Scheduler priority
    uint8_t max_count;          // Maximal count for variable frequency scheduling
    size_t time_used;           // CPU time consumed
    size_t xstate;              // Address for extended state context (mainly FPU registers)
    size_t rsp0;                // Top address for kernel space stack
    union {
        size_t rsp;             // Current position in stack
        stack_t *stack;         // Stack as struct
    };
    process_t *parent;          // Parent process
    link_t plink;               // Link for threads in parent process
    link_t slink;               // Link for threads in scheduler queue
    link_t wlink;               // Link for threads in wait queue
    wq_t *wq;                   // Current wait queue
    spinlock_t *lock;           // Optional lock used for safe thread blocking
    yield_t yield;              // Thread is currently yielding
    size_t signals;             // List of signals
    struct {                    // Used for IRQ signals
        int recv;               // Signal received
        int wait;               // Waiting for signal
        spinlock_t lock;        // Synchronization lock
    } sig;
    struct {                    // Used in swapgs
        size_t krsp;            // Kernel RSP
        size_t ursp;            // User RSP
    } gs;
};

struct process {
    char name[32];        // Process name
    int exitcode;         // Exit code when process has terminated
    state_t state;        // Process state (running or terminated)
    pid_t pid;            // Process ID
    pid_t next_tid;       // Next thread ID
    size_t pml4;          // Physical address of PML4
    uint32_t uid;         // User ID
    uint32_t gid;         // Group ID
    dentry_t *cwd;        // Current working directory
    process_t *parent;    // Parent process
    list_t threads;       // List of threads
    list_t children;      // List of children
    link_t clink;         // Link in child processes
    link_t plink;         // Link in global processes
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
