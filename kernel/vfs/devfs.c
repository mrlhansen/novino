#include <kernel/vfs/devfs.h>
#include <kernel/vfs/vfs.h>
#include <kernel/mem/heap.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>

static int mounted = 0;
static devfs_t *root = 0;

static devfs_t *devfs_alloc(const char *name, int flags)
{
    devfs_t *entry;
    inode_t *inode;

    if(name)
    {
        // vfs_validate_name
    }

    entry = kzalloc(sizeof(devfs_t));
    if(entry == 0)
    {
        return 0;
    }

    if(name)
    {
        strcpy(entry->name, name);
    }

    inode = &entry->inode;
    inode->obj = entry;
    inode->ino = 0;
    inode->flags = flags;
    inode->links = 1;

    if(flags == I_DIR)
    {
        inode->mode = 0755;
    }
    else
    {
        inode->mode = 0644;
    }

    inode->mtime = 0; // TODO
    inode->ctime = 0; // TODO
    inode->atime = 0; // TODO

    return entry;
}

static devfs_t *devfs_find(devfs_t *parent, const char *name)
{
    devfs_t *curr;
    curr = parent->child;

    while(curr)
    {
        if(strcmp(curr->name, name) == 0)
        {
            return curr;
        }
        curr = curr->next;
    }

    return 0;
}

devfs_t *devfs_register(devfs_t *parent, const char *name, devfs_ops_t *ops, void *data, int flags, stat_t *stat)
{
    devfs_t *entry;
    inode_t *inode;

    if(flags != I_STREAM && flags != I_BLOCK)
    {
        return 0;
    }

    if(parent == 0)
    {
        parent = root;
    }

    if(parent->inode.flags != I_DIR)
    {
        return 0;
    }

    if(devfs_find(parent, name))
    {
        return 0;
    }

    entry = devfs_alloc(name, flags);
    if(entry == 0)
    {
        return 0;
    }

    if(stat)
    {
        inode = &entry->inode;
        if(stat->mode)
        {
            inode->mode = stat->mode;
        }
        if(stat->uid)
        {
            inode->uid = stat->uid;
        }
        if(stat->gid)
        {
            inode->gid = stat->gid;
        }
        if(stat->size)
        {
            inode->size = stat->size;
        }
        if(stat->blksz)
        {
            inode->blksz = stat->blksz;
        }
        if(stat->blocks)
        {
            inode->blocks = stat->blocks;
        }
    }

    entry->data = data;
    entry->ops = ops;
    entry->child = 0;
    entry->next = 0;

    entry->next = parent->child;
    parent->child = entry;

    return entry;
}

devfs_t *devfs_mkdir(devfs_t *parent, const char *name)
{
    devfs_t *entry;

    if(parent == 0)
    {
        parent = root;
    }

    if(parent->inode.flags != I_DIR)
    {
        return 0;
    }

    if(devfs_find(parent, name))
    {
        return 0;
    }

    entry = devfs_alloc(name, I_DIR);
    if(entry == 0)
    {
        return 0;
    }

    entry->next = parent->child;
    parent->child = entry;

    return entry;
}

static int devfs_open(file_t *file)
{
    devfs_ops_t *ops;
    devfs_t *entry;

    entry = file->inode->obj;
    ops = entry->ops;

    if(ops && ops->open)
    {
        file->data = entry->data;
        return ops->open(file);
    }

    return 0;
}

static int devfs_close(file_t *file)
{
    devfs_ops_t *ops;
    devfs_t *entry;

    entry = file->inode->obj;
    ops = entry->ops;

    if(ops && ops->close)
    {
        return ops->close(file);
    }

    return 0;
}

static int devfs_read(file_t *file, size_t size, void *buf)
{
    devfs_ops_t *ops;
    devfs_t *entry;

    entry = file->inode->obj;
    ops = entry->ops;

    if(ops->read == 0)
    {
        return -ENOTSUP;
    }

    return ops->read(file, size, buf);
}

static int devfs_write(file_t *file, size_t size, void *buf)
{
    devfs_ops_t *ops;
    devfs_t *entry;

    entry = file->inode->obj;
    ops = entry->ops;

    if(ops->write == 0)
    {
        return -ENOTSUP;
    }

    return ops->write(file, size, buf);
}

static int devfs_seek(file_t *file, long offset, int origin)
{
    devfs_ops_t *ops;
    devfs_t *entry;

    entry = file->inode->obj;
    ops = entry->ops;

    if(ops->seek == 0)
    {
        return -ENOTSUP;
    }

    return ops->seek(file, offset, origin);
}

static int devfs_ioctl(file_t *file, size_t cmd, size_t val)
{
    devfs_ops_t *ops;
    devfs_t *entry;

    entry = file->inode->obj;
    ops = entry->ops;

    if(ops->ioctl == 0)
    {
        return -ENOTSUP;
    }

    return ops->ioctl(file, cmd, val);
}

static int devfs_readdir(file_t *file, void *data)
{
    devfs_t *parent, *curr;

    parent = file->inode->obj;
    curr = parent->child;

    while(curr)
    {
        vfs_filler(data, curr->name, &curr->inode);
        curr = curr->next;
    }

    return 0;
}

static int devfs_lookup(inode_t *ip, const char *name, inode_t *inode)
{
    devfs_t *parent, *entry;

    parent = ip->obj;
    entry = devfs_find(parent, name);
    if(entry == 0)
    {
        return -ENOENT;
    }

    *inode = entry->inode;
    return 0;
}

static void *devfs_mount(devfs_t *dev, inode_t *inode)
{
    if(mounted)
    {
        return 0;
    }

    mounted = 1;
    *inode = root->inode;

    return root;
}

static int devfs_umount(void *data)
{
    mounted = 0;
    return 0;
}

void devfs_init()
{
    static vfs_ops_t ops = {
        .open = devfs_open,
        .close = devfs_close,
        .read = devfs_read,
        .write = devfs_write,
        .seek = devfs_seek,
        .ioctl = devfs_ioctl,
        .readdir = devfs_readdir,
        .lookup = devfs_lookup,
        .mount = devfs_mount,
        .umount = devfs_umount
    };

    root = devfs_alloc(0, I_DIR);
    if(root == 0)
    {
        kp_panic("devfs", "unable to alloc devfs root node");
    }

    vfs_register("devfs", &ops);
}
