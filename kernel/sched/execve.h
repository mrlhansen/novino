#pragma once

#include <kernel/sched/threads.h>
#include <kernel/vfs/fd.h>

typedef struct {
    char **argv;
    char **envp;
    fd_t *ofd;
    fd_t *ifd;
} execve_t;

void switch_to_user_mode(size_t rip, size_t rsp, size_t argv, size_t envp);
pid_t execve(const char *filename, char **argv, char **envp, int stdin, int stdout);
