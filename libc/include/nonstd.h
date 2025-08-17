#pragma once

#include <stdio.h>

struct stat {
    size_t st_dev;
    size_t st_ino;
    size_t st_mode;
    size_t st_nlink;
    size_t st_uid;
    size_t st_gid;
    size_t st_rdev;
    size_t st_size;
    size_t st_atime;
    size_t st_mtime;
    size_t st_ctime;
    size_t st_blksize;
    size_t st_blocks;
};

int stat(const char *path, struct stat *buf);

long spawnvef(const char *pathname, char *const argv[], char *const envp[], FILE *stdin, FILE *stdout);
long spawnve(const char *pathname, char *const argv[], char *const envp[]);
long spawnv(const char *pathname, char *const argv[]);

long wait(long pid, int *status);
