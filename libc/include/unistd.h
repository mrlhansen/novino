#pragma once

#include <stddef.h>

extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;

typedef long pid_t;

int getopt(int argc, char * const argv[], const char *optstring);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
pid_t getpid();
