#pragma once

#include <kernel/sched/threads.h>
#include <kernel/vfs/types.h>
#include <kernel/sysinfo.h>
#include <kernel/lists.h>

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
    link_t sibling;       // Link in child processes
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

void process_exit(int status);
pid_t process_wait(pid_t pid, int *status);

void process_append_thread(process_t *parent, thread_t *thread);
void process_remove_thread(thread_t *thread);

process_t *process_create(const char *name, uint64_t pml4, process_t *parent);
process_t *process_handle();

void process_idle_cleaning();

void sysinfo_proclist(sysinfo_t *sys);
void sysinfo_procinfo(sysinfo_t *sys, size_t pid);
