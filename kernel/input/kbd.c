#include <kernel/input/input.h>
#include <kernel/vfs/devfs.h>
#include <kernel/vfs/pipes.h>
#include <kernel/time/time.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <kernel/lists.h>

static spinlock_t lock;
static LIST_INIT(pipes, pipe_t, link);

static int input_kbd_open(file_t *file)
{
    pipe_t *pipe;

    pipe = pipe_create(0, O_NONBLOCK);
    if(pipe == 0)
    {
        return -EIO;
    }

    file->data = pipe;
    list_insert(&pipes, file->data);
    return 0;
}

static int input_kbd_close(file_t *file)
{
    list_remove(&pipes, file->data);
    pipe_delete(file->data);
    return 0;
}

static int input_kbd_read(file_t *file, size_t size, void *buf)
{
    return pipe_read(file->data, size, buf);
}

void input_kbd_write(int code, int value)
{
    pipe_t *pipe = pipes.head;
    input_event_t event = {
        .ts = system_timestamp(),
        .type = 0, // EV_KEY
        .code = code,
        .value = value,
    };

    while(pipe)
    {
        pipe_write(pipe, sizeof(event), &event);
        pipe = pipe->link.next;
    }
}

void input_kbd_init()
{
    static devfs_ops_t ops = {
        .open = input_kbd_open,
        .close = input_kbd_close,
        .read = input_kbd_read,
        .write = 0,
        .seek = 0,
        .ioctl = 0, // use this to control keyboard leds? We need to register keyboards with a callback function
    };
    devfs_register(0, "keyboard", &ops, 0, I_STREAM, 0);
}
