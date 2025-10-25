#include <kernel/vfs/pipes.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <kernel/debug.h>

static inline void wake(pipe_io_t *io)
{
    acquire_lock(&io->lock);
    if(io->thread)
    {
        thread_unblock(io->thread);
        io->thread = 0;
    }
    release_lock(&io->lock);
}

static inline void sleep(pipe_io_t *io)
{
    acquire_lock(&io->lock);
    io->thread = thread_handle();
    thread_block(&io->lock);
}

pipe_t *pipe_create(int rflags, int wflags)
{
    pipe_t *pipe;

    pipe = kzalloc(sizeof(pipe_t));
    if(pipe == 0)
    {
        return 0;
    }

    pipe->rd.mutex = create_mutex();
    pipe->rd.flags = rflags;
    pipe->wr.mutex = create_mutex();
    pipe->wr.flags = wflags;

    return pipe;
}

int pipe_delete(pipe_t *pipe)
{
    pipe_io_t *rd = &pipe->rd;
    pipe_io_t *wr = &pipe->wr;

    if(rd->lock || rd->thread)
    {
        return -EBUSY;
    }

    if(wr->lock || wr->thread)
    {
        return -EBUSY;
    }

    free_mutex(rd->mutex); // this can fail
    free_mutex(wr->mutex); // this can fail
    kfree(pipe);

    return 0;
}

int pipe_write(pipe_t *pipe, int maxlen, void *data)
{
    pipe_io_t *rd = &pipe->rd;
    pipe_io_t *wr = &pipe->wr;
    uint8_t *buf = data;
    bool status;
    int length = 0;
    int next;

    status = (wr->flags & O_NONBLOCK) ? true : false;
    status = acquire_mutex(wr->mutex, status);
    if(!status)
    {
        return 0;
    }

    // Write to buffer
    while(maxlen--)
    {
        next = (pipe->head + 1) % PIPE_BUFFER_SIZE;

        // Buffer is full
        if(next == pipe->tail)
        {
            if(wr->flags & O_NONBLOCK)
            {
                break;
            }
            wake(rd);
            sleep(wr);
        }

        pipe->data[pipe->head] = *buf++;
        pipe->head = next;
        length++;
    }

    wake(rd);
    release_mutex(wr->mutex);

    return length;
}

int pipe_read(pipe_t *pipe, int maxlen, void *data)
{
    pipe_io_t *rd = &pipe->rd;
    pipe_io_t *wr = &pipe->wr;
    uint8_t *buf = data;
    bool status;
    int length = 0;
    int next;

    status = (rd->flags & O_NONBLOCK) ? true : false;
    status = acquire_mutex(rd->mutex, status);
    if(!status)
    {
        return 0;
    }

    // Buffer is empty
    if(pipe->head == pipe->tail)
    {
        if(rd->flags & O_NONBLOCK)
        {
            release_mutex(rd->mutex);
            return 0;
        }
        sleep(rd);
    }

    // Read from buffer
    while(maxlen--)
    {
        next = (pipe->tail + 1) % PIPE_BUFFER_SIZE;

        // Buffer is empty
        if(pipe->head == pipe->tail)
        {
            break;
        }

        *buf++ = pipe->data[pipe->tail];
        pipe->tail = next;
        length++;
    }

    wake(wr);
    release_mutex(rd->mutex);

    return length;
}
