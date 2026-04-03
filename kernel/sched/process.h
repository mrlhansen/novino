#pragma once

#include <kernel/sched/types.h>
#include <kernel/sysinfo.h>

int process_kill(pid_t pid);
void process_exit(int status);
pid_t process_wait(pid_t pid, int *status);

void process_append_thread(process_t *parent, thread_t *thread);
void process_remove_thread(thread_t *thread);

process_t *process_create(const char *name, uint64_t pml4, process_t *parent);
process_t *process_handle();

void process_idle_cleaning();

void sysinfo_proclist(sysinfo_t *sys);
void sysinfo_procinfo(sysinfo_t *sys, size_t pid);
