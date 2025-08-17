#pragma once

#include <stdio.h>
#include <errno.h>

#define UNGETSIZ 8

enum {
    F_READ    = (1 << 0), // Read
    F_WRITE   = (1 << 1), // Write
    F_APPEND  = (1 << 2), // Append
    F_TEXT    = (1 << 3), // Text mode stream (stdin and stdout)
    F_EOF     = (1 << 4), // End of file
    F_ERR     = (1 << 5), // Error
    F_DIR     = (1 << 6), // Directory
};

struct __libc_fd {
    int fd;     // file descriptor
    int flags;  // flags
    int mode;   // current mode (read or write)
    struct {
        char *ptr;   // buffer start
        char *pos;   // buffer position (for next read or write)
        char *end;   // buffer end (end of buffered data in read mode, end of total buffer size in write mode)
        int size;    // buffer size
        int mode;    // buffer mode
    } b;
    struct {
        char *ptr; // ungetc buffer
        char *pos; // ungetc buffer position
    } u;
    FILE *prev; // linked list prev
    FILE *next; // linked list next
};

FILE *__libc_fd_alloc(int fd);
void __libc_fd_free(FILE *fp);
void __libc_fd_exit();

long __libc_fp_read(FILE *fp, size_t size, void *ptr);
long __libc_fp_write(FILE *fp, size_t size, void *ptr);
long __libc_fp_flush(FILE *fp);
