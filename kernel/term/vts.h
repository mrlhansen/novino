#pragma once

#include <kernel/term/console.h>
#include <kernel/vfs/pipes.h>

typedef struct {
    int id;              // ID
    int rdwait;          // Read operation in progess
    file_t *file;        // Open file descriptor
    pipe_t *pipe;        // Pipe for reading
    console_t *console;  // Console
    int flags;           // Terminal flags
    int ipos;            // Terminal input buffer position
    char ibuf[128];      // Terminal input buffer data
} vts_t;

void vts_flush_input(vts_t *vts);
vts_t *vts_select(int num);
void vts_init();
