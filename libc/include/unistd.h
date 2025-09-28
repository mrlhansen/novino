#pragma once

#include <stddef.h>

extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;

typedef long pid_t;
pid_t getpid();

int getopt(int argc, char * const argv[], const char *optstring);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);

int create(const char *path, int mode);
int ioctl(int fd, size_t op, ...);

int mkdir(const char *path, int mode);
int rmdir(const char *path);

ssize_t read(int fd, void *buf, size_t size);
ssize_t write(int fd, void *buf, size_t size);
