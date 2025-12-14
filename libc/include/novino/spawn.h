#pragma once

#include <unistd.h>
#include <stdio.h>

pid_t spawnvef(const char *pathname, char *const argv[], char *const envp[], int ifd, int ofd);
pid_t spawnve(const char *pathname, char *const argv[], char *const envp[]);
pid_t spawnv(const char *pathname, char *const argv[]);

pid_t wait(pid_t pid, int *status);
