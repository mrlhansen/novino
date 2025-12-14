#pragma once

#include <kernel/vfs/types.h>

typedef struct {
    int id;       // File descriptor ID (per process)
    file_t *file; // File context
    link_t link;  // Link to next
} fd_t;

fd_t *fd_create();
fd_t *fd_find(int id);
file_t *fd_delete(fd_t *fd);

fd_t *fd_clone(fd_t *fd);
void fd_adopt(fd_t *fd);
