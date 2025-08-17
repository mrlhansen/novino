#pragma once

#include <kernel/sched/threads.h>
#include <kernel/vfs/fd.h>

typedef struct {
    char **argv;
    char **envp;
    file_t *ofd;
    file_t *ifd;
} execve_t;

void switch_to_user_mode(size_t, size_t, size_t, size_t);
pid_t execve(const char *filename, char **argv, char **envp, int stdin, int stdout);
