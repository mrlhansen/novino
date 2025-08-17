#pragma once

#include <kernel/sched/process.h>
#include <kernel/sched/mutex.h>
#include <kernel/lists.h>

#define PIPE_BUFFER_SIZE (4*1024+1)

typedef struct {
    uint32_t flags;
    mutex_t *mutex;
    spinlock_t lock;
    thread_t *thread;
} pipe_io_t;

typedef struct _pipe {
    link_t link;                    // Link for list of pipes
    pipe_io_t rd;                   // Reading
    pipe_io_t wr;                   // Writing
    volatile uint32_t head;         // Buffer head
    volatile uint32_t tail;         // Buffer tail
    uint8_t data[PIPE_BUFFER_SIZE]; // Buffer data
} pipe_t;

pipe_t *pipe_create(int rflags, int wflags);
int pipe_delete(pipe_t*);
int pipe_read(pipe_t*, int, void*);
int pipe_write(pipe_t*, int, void*);
