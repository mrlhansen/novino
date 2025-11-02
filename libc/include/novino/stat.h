#pragma once

#include <stddef.h>
#include <time.h>

#define S_IFREG 0x01000
#define S_IFDIR 0x02000
#define S_IFBLK 0x04000
#define S_IFSTR 0x08000
#define S_IFLNK 0x10000

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
