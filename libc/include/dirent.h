#pragma once

#include <stddef.h>

typedef struct __libc_fd DIR;

struct dirent {
    size_t d_ino;
    unsigned char d_type;
    char d_name[256];
};

#define DT_UNKNOWN  0  // Unknown file type
#define DT_BLK      1  // Block device
#define DT_CHR      2  // Character device
#define DT_DIR      3  // Directory
#define DT_FIFO     4  // Named FIFO
#define DT_LNK      5  // Symbolic link
#define DT_REG      6  // Regular file
#define DT_SOCK     7  // Socket

DIR *opendir(const char *dirname);
int closedir(DIR *dp);
struct dirent *readdir(DIR *dp);
