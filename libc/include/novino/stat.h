#pragma once

#include <stddef.h>
#include <time.h>

struct stat {
    size_t st_dev;
    size_t st_ino;
    size_t st_mode;
    size_t st_nlink;
    size_t st_uid;
    size_t st_gid;
    size_t st_rdev;
    size_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    size_t st_blksize;
    size_t st_blocks;
};

int stat(const char *path, struct stat *buf);
