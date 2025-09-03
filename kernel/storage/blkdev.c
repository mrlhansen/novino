#include <kernel/storage/blkdev.h>
#include <kernel/vfs/types.h>
#include <kernel/mem/heap.h>
#include <kernel/errno.h>

int blkdev_open(devfs_t *dev)
{
    blkdev_t bd;
    int status;

    // attempt to lock device
    status = atomic_lock(&dev->lock);
    if(status)
    {
        return -EBUSY;
    }

    // check status
    status = dev->blk->status(dev->data, &bd, 1);
    if(status < 0)
    {
        atomic_unlock(&dev->lock);
        return status;
    }

    // allocate data
    dev->bd = kzalloc(sizeof(blkdev_t));
    if(dev->bd == 0)
    {
        atomic_unlock(&dev->lock);
        return -ENOMEM;
    }

    // update devfs node
    dev->inode.blksz  = bd.bps;
    dev->inode.blocks = bd.sectors;
    dev->inode.size   = bd.bps * bd.sectors;
    dev->bd[0]        = bd;

    return 0;
}

void blkdev_close(devfs_t *dev)
{
    kfree(dev->bd);
    dev->bd = 0;
    atomic_unlock(&dev->lock);
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
