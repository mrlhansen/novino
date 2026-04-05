#include <kernel/vfs/pipes.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <kernel/debug.h>

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

    wq_init(&pipe->rd.queue);
    wq_init(&pipe->wr.queue);

    return pipe;
}

int pipe_delete(pipe_t *pipe)
{
    pipe_io_t *rd = &pipe->rd;
    pipe_io_t *wr = &pipe->wr;

    if(wq_size(&rd->queue) > 0)
    {
        return -EBUSY;
    }

    if(wq_size(&wr->queue) > 0)
    {
        return -EBUSY;
    }

    free_mutex(rd->mutex); // this can fail
    free_mutex(wr->mutex); // this can fail
    kfree(pipe);

    return 0;
}

bool pipe_empty(pipe_t *pipe)
{
    if(pipe->head == pipe->tail)
    {
        return true;
    }
    return false;
}

int pipe_write(pipe_t *pipe, int maxlen, void *data)
{
    pipe_io_t *rd = &pipe->rd;
    pipe_io_t *wr = &pipe->wr;
    uint8_t *buf = data;
    bool nonblock;
    int length = 0;
    int status;
    int next;

    nonblock = (wr->flags & O_NONBLOCK) ? true : false;
    status = acquire_mutex(wr->mutex, nonblock);
    if(status < 0)
    {
        return status;
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

            wq_wake(&rd->queue);

            status = wq_wait(&wr->queue);
            if(status < 0)
            {
                release_mutex(wr->mutex);
                return status;
            }
        }

        pipe->data[pipe->head] = *buf++;
        pipe->head = next;
        length++;
    }

    wq_wake(&rd->queue);
    release_mutex(wr->mutex);

    return length;
}

int pipe_read(pipe_t *pipe, int maxlen, void *data)
{
    pipe_io_t *rd = &pipe->rd;
    pipe_io_t *wr = &pipe->wr;
    uint8_t *buf = data;
    bool nonblock;
    int length = 0;
    int status;
    int next;

    nonblock = (rd->flags & O_NONBLOCK) ? true : false;
    status = acquire_mutex(rd->mutex, nonblock);
    if(status < 0)
    {
        return status;
    }

    // Buffer is empty
    if(pipe->head == pipe->tail)
    {
        if(rd->flags & O_NONBLOCK)
        {
            release_mutex(rd->mutex);
            return 0;
        }

        status = wq_wait(&rd->queue);
        if(status < 0)
        {
            release_mutex(rd->mutex);
            return status;
        }
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

    wq_wake(&wr->queue);
    release_mutex(rd->mutex);

    return length;
}
