#include <kernel/storage/partmgr.h>
#include <kernel/storage/blkdev.h>
#include <kernel/vfs/devfs.h>
#include <kernel/mem/heap.h>
#include <kernel/cleanup.h>
#include <kernel/errno.h>
#include <stdio.h>

DEFINE_AUTOFREE_TYPE(gpt_header_t)
DEFINE_AUTOFREE_TYPE(gpt_entry_t)
DEFINE_AUTOFREE_TYPE(uint8_t)

static int partmgr_register(devfs_t *dev, int id, size_t offset, size_t size)
{
    devfs_t *subdev;
    char name[16];

    if(dev->bd->sectors < offset+size)
    {
        return -EINVAL;
    }

    sprintf(name, "%sp%d", dev->name, id);
    subdev = devfs_block_register(dev->parent, name, dev->blk, dev->data, 0);
    if(subdev)
    {
        blkdev_alloc(subdev, offset, size, false);
    }

    return 0;
}

static int gpt_table(devfs_t *dev)
{
    autofree(gpt_header_t) *hdr = 0;
    autofree(uint8_t) *buf = 0;
    gpt_entry_t *entry;
    size_t size;
    int status;
    int count;

    hdr = kmalloc(dev->bd->bps);
    if(!hdr)
    {
        return -ENOMEM;
    }

    status = blkdev_read(dev, 1, 1, hdr);
    if(status < 0)
    {
        return status;
    }

    count = 1 + (hdr->pa_count * hdr->pa_size) / dev->bd->bps;
    buf = kcalloc(count, dev->bd->bps);
    if(!buf)
    {
        return -ENOMEM;
    }

    status = blkdev_read(dev, hdr->pa_lba, count, buf);
    if(status < 0)
    {
        return status;
    }

    entry = (void*)buf;

    for(int n = 0; n < hdr->pa_count; n++)
    {
        if(entry->lba_first && entry->lba_last)
        {
            size = entry->lba_last - entry->lba_first + 1;
            status = partmgr_register(dev, n, entry->lba_first, size);
            if(status < 0)
            {
                return status;
            }
        }
        entry++;
    }

    return 0;
}

static int mbr_extended(devfs_t *dev, int id, size_t start)
{
    autofree(uint8_t) *buf = 0;
    mbr_entry_t *part;
    mbr_entry_t *next;
    int status;

    buf = kmalloc(dev->bd->bps);
    if(!buf)
    {
        return -ENOMEM;
    }

    part = (mbr_entry_t*)(buf + 0x1BE);
    next = part + 1;

    status = blkdev_read(dev, start, 1, buf);
    if(status < 0)
    {
        return status;
    }

    if(part->type && part->lba_size)
    {
        status = partmgr_register(dev, id, start + part->lba_start, part->lba_size);
        if(status < 0)
        {
            return status;
        }
    }

    if((next->type == 0x05) || (next->type == 0x0F))
    {
        status = mbr_extended(dev, id+1, start + next->lba_start);
        if(status < 0)
        {
            return status;
        }
    }

    return 0;
}

int partmgr_init(devfs_t *dev)
{
    autofree(uint8_t) *buf = 0;
    mbr_entry_t *part;
    uint16_t signature;
    int status;

    // check that device is ready
    status = blkdev_open(dev);
    if(status < 0)
    {
        return status;
    }
    blkdev_close(dev);

    buf = kmalloc(dev->bd->bps);
    if(!buf)
    {
        return -ENOMEM;
    }

    status = blkdev_read(dev, 0, 1, buf);
    if(status < 0)
    {
        return status;
    }

    part = (mbr_entry_t*)(buf + 0x1BE);
    signature = *(uint16_t*)(buf + 0x1FE);

    if(signature != 0xAA55)
    {
        return 0;
    }

    if(part->type == 0xEE)
    {
        return gpt_table(dev);
    }

    for(int n = 0; n < 4; n++)
    {
        if((part->type == 0x05) || (part->type == 0x0F))
        {
            status = mbr_extended(dev, 4, part->lba_start);
            if(status < 0)
            {
                return status;
            }
        }
        else if(part->type && part->lba_size)
        {
            status = partmgr_register(dev, n, part->lba_start, part->lba_size);
            if(status < 0)
            {
                return status;
            }
        }
        part++;
    }

    return 0;
}
