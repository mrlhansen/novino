#pragma once

#include <stdio.h>

long spawnvef(const char *pathname, char *const argv[], char *const envp[], FILE *stdin, FILE *stdout);
long spawnve(const char *pathname, char *const argv[], char *const envp[]);
long spawnv(const char *pathname, char *const argv[]);

long wait(long pid, int *status);
