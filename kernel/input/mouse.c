#include <kernel/input/input.h>
#include <kernel/vfs/devfs.h>

void input_mouse_init()
{
    static devfs_ops_t ops = {
        .open = 0,
        .close = 0,
        .read = 0,
        .write = 0,
        .seek = 0,
        .ioctl = 0,
    };
    devfs_register(0, "mouse", &ops, 0, I_STREAM, 0);
}
