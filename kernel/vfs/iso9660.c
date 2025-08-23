#include <kernel/vfs/vfs.h>

void *iso9660_mount(devfs_t *dev, inode_t *root)
{

}

int iso9660_umount(void *data)
{

}

void iso9660_init()
{
    static vfs_ops_t ops = {
        .open = 0,
        .close = 0,
        .read = 0,
        .write = 0,
        .seek = 0,
        .ioctl = 0,
        .readdir = 0,
        .lookup = 0,
        .mount = iso9660_mount,
        .umount = iso9660_umount
    };
    vfs_register("iso9660", &ops);
}
