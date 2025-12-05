#include <kernel/input/input.h>
#include <kernel/vfs/devfs.h>
#include <kernel/vfs/pipes.h>
#include <kernel/time/time.h>
#include <kernel/errno.h>
#include <kernel/lists.h>

static LIST_INIT(pipes, pipe_t, link);

static int input_mouse_open(file_t *file)
{
    int rflags = 0;
    pipe_t *pipe;

    if(file->flags & O_NONBLOCK)
    {
        rflags = O_NONBLOCK;
    }

    pipe = pipe_create(rflags, O_NONBLOCK);
    if(!pipe)
    {
        return -EIO;
    }

    file->data = pipe;
    list_insert(&pipes, file->data);
    return 0;
}

static int input_mouse_close(file_t *file)
{
    list_remove(&pipes, file->data);
    pipe_delete(file->data);
    return 0;
}

static int input_mouse_read(file_t *file, size_t size, void *buf)
{
    return pipe_read(file->data, size, buf);
}

void input_mouse_write(int type, int code, int value)
{
    pipe_t *pipe = pipes.head;
    input_event_t event = {
        .ts = system_timestamp(),
        .type = type,
        .code = code,
        .value = value,
    };

    while(pipe)
    {
        pipe_write(pipe, sizeof(event), &event);
        pipe = pipe->link.next;
    }
}

void input_mouse_init()
{
    static devfs_ops_t ops = {
        .open = input_mouse_open,
        .close = input_mouse_close,
        .read = input_mouse_read,
        .write = 0,
        .seek = 0,
        .ioctl = 0,
    };
    devfs_stream_register(0, "mouse", &ops, 0, 0);
}
