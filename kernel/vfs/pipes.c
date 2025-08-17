#include <kernel/vfs/pipes.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>
#include <kernel/debug.h>

// RETHINK THE USE OF MUTEXES

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
    int length = 0;
    int next;

    if((wr->flags & O_NONBLOCK) && (wr->mutex->free == 0)) // do we need a function for this that acquires a lock?
    {
        return 0;
    }

    acquire_mutex(wr->mutex); // or we need a flag that tells wether to block or just return with an error

    // Write to buffer
    while(maxlen--)
    {
        next = (pipe->head + 1) % PIPE_BUFFER_SIZE;
        if(next == pipe->tail) // Buffer is full
        {
            // Blocking?
            if(wr->flags & O_NONBLOCK)
            {
                break;
            }

            // Wake reader?
            acquire_lock(&rd->lock);
            if(rd->thread)
            {
                thread_unblock(rd->thread);
                rd->thread = 0;
            }
            release_lock(&rd->lock);

            // Sleep
            acquire_lock(&wr->lock);
            wr->thread = thread_handle();
            thread_block(&wr->lock);
        }

        pipe->data[pipe->head] = *buf++;
        pipe->head = next;
        length++;
    }

    // Wake reader?
    acquire_lock(&rd->lock);
    if(rd->thread)
    {
        thread_unblock(rd->thread);
        rd->thread = 0;
    }
    release_lock(&rd->lock);

    release_mutex(wr->mutex);
    return length;
}

int pipe_read(pipe_t *pipe, int maxlen, void *data)
{
    pipe_io_t *rd = &pipe->rd;
    pipe_io_t *wr = &pipe->wr;
    uint8_t *buf = data;
    int length = 0;
    int next;

    acquire_mutex(rd->mutex);

    // Empty buffer?
    if(pipe->head == pipe->tail)
    {
        if(rd->flags & O_NONBLOCK)
        {
            release_mutex(rd->mutex);
            return 0;
        }

        acquire_lock(&rd->lock);
        rd->thread = thread_handle();
        thread_block(&rd->lock);
    }

    // Read from buffer
    while(maxlen--)
    {
        next = (pipe->tail + 1) % PIPE_BUFFER_SIZE;
        if(pipe->head == pipe->tail) // Buffer is empty
        {
            break;
        }

        *buf++ = pipe->data[pipe->tail];
        pipe->tail = next;
        length++;
    }

    // Wake writer?
    acquire_lock(&wr->lock);
    if(wr->thread)
    {
        thread_unblock(wr->thread);
        wr->thread = 0;
    }
    release_lock(&wr->lock);

    release_mutex(rd->mutex);
    return length;
}
