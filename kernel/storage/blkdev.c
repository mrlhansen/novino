#include <kernel/storage/blkdev.h>
#include <kernel/storage/partmgr.h>
#include <kernel/vfs/types.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>

int blkdev_alloc(devfs_t *dev, size_t offset, size_t size, bool partitionable)
{
    blkdev_t *bd;
    inode_t *inode;
    int status;

    // allocate data
    dev->bd = kzalloc(sizeof(blkdev_t));
    if(dev->bd == 0)
    {
        return -ENOMEM;
    }

    bd = dev->bd;
    inode = &dev->inode;

    if(!size)
    {
        bd->dynsz = true;
    }

    // check status
    status = dev->blk->status(dev->data, bd, 1);
    if(status < 0)
    {
        if(size)
        {
            return status;
        }

        if(bd->flags & BLKDEV_REMOVEABLE)
        {
            return 0;
        }

        return status;
    }

    // update information
    if(bd->dynsz)
    {
        size = bd->sectors;
    }

    bd->offset = offset;
    bd->size = size;

    inode->blksz  = bd->bps;
    inode->blocks = bd->size;
    inode->size   = bd->bps * bd->size;

    // partitions
    if(partitionable)
    {
        partmgr_init(dev);
    }

    return 0;
}

int blkdev_free(devfs_t *dev)
{
    blkdev_t *bd;
    int status;

    bd = dev->bd;
    status = atomic_lock(&bd->lock);
    if(status)
    {
        return -EBUSY;
    }

    kfree(bd);
    return 0;
}

int blkdev_open(devfs_t *dev) // flags, like exclusive?
{
    blkdev_t *bd;
    inode_t *inode;
    int status;

    bd = dev->bd;
    inode = &dev->inode;

    // attempt to lock device
    status = atomic_lock(&bd->lock);
    if(status)
    {
        return -EBUSY;
    }

    // check status
    status = dev->blk->status(dev->data, bd, 1);
    if(status < 0)
    {
        atomic_unlock(&bd->lock);
        return status;
    }

    // update information
    if(bd->dynsz)
    {
        bd->size = bd->sectors;
    }

    inode->blksz  = bd->bps;
    inode->blocks = bd->size;
    inode->size   = bd->bps * bd->size;

    return 0;
}

void blkdev_close(devfs_t *dev)
{
    atomic_unlock(&dev->bd->lock);
}

static inline int blkdev_rw(devfs_t *dev, size_t offset, size_t count, void *data, int write)
{
    blkdev_t *bd = dev->bd;

    offset = offset + bd->offset;
    if(offset > bd->sectors)
    {
        return -EINVAL;
    }
    if(offset + count > bd->sectors)
    {
        return -EINVAL;
    }

    if(write)
    {
        return dev->blk->write(dev->data, offset, count, data);
    }
    else
    {
        return dev->blk->read(dev->data, offset, count, data);
    }
}

int blkdev_read(devfs_t *dev, size_t offset, size_t count, void *data)
{
    return blkdev_rw(dev, offset, count, data, 0);
}

int blkdev_write(devfs_t *dev, size_t offset, size_t count, void *data)
{
    return blkdev_rw(dev, offset, count, data, 1);
}
