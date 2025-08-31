#include <kernel/storage/libata.h>
#include <kernel/mem/heap.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>
#include <stdio.h>

static void ata_copy_string(char *dst, uint16_t *src, int len)
{
    for(int i = 0; i < len; i++)
    {
        dst[2*i] = (src[i] >> 8);
        dst[2*i+1] = (src[i] & 0xFF);
    }
    dst[2*len] = '\0';
    strtrim(dst, dst);
}

void libata_identify(uint16_t *info, ata_info_t *ret)
{
    uint32_t flags, sectors, lss, pss;
    uint8_t revision, atapi;

    // Set variables
    sectors = 0;
    revision = 0;
    flags = 0;
    atapi = 0;

    // Copy strings
    ata_copy_string(ret->serial, info+10, 10);
    ata_copy_string(ret->firmware, info+23, 4);
    ata_copy_string(ret->model, info+27, 20);

    // ATA revision
    for(int i = 0; i < 10; i++)
    {
        if(info[80] & (1 << i))
        {
            revision = i;
        }
    }

    // ATAPI or ATA device
    if(info[0] & 0x8000)
    {
        lss = 2048;
        pss = 2048;
        atapi = 1;

        if(revision < 7)
        {
            if(info[49] & (1 << 8))
            {
                flags |= ATA_FLAG_DMA;
            }

            if(info[53] & (1 << 2))
            {
                if(info[88] & 0x0F)
                {
                    flags |= ATA_FLAG_UDMA;
                }
            }
        }
        else
        {
            if(info[62] & (1 << 15))
            {
                flags |= ATA_FLAG_DMADIR;
            }

            if(info[62] & (1 << 10))
            {
                flags |= ATA_FLAG_DMA;
            }

            if(info[62] & 0x7F)
            {
                flags |= ATA_FLAG_UDMA;
            }
        }
    }
    else
    {
        lss = 512;
        pss = 512;

        if(info[83] & (1 << 10))
        {
            flags |= ATA_FLAG_LBA48;
        }

        if(info[49] & (1 << 9))
        {
            flags |= ATA_FLAG_LBA28;
        }

        if(info[49] & (1 << 8))
        {
            flags |= ATA_FLAG_DMA;
        }

        if(info[53] & (1 << 2))
        {
            if(info[88] & 0x7F)
            {
                flags |= ATA_FLAG_UDMA;
            }
        }

        if(info[106] & (1 << 12))
        {
            lss = info[118];
            lss = (lss << 16) | info[117];
            pss = lss;
        }

        if(info[106] & (1 << 13))
        {
            pss = (info[106] & 0x0F);
            pss = (1 << pss) * lss;
        }

        if(flags & ATA_FLAG_LBA48)
        {
            sectors = info[103];
            sectors = (sectors << 16) | info[102];
            sectors = (sectors << 16) | info[101];
            sectors = (sectors << 16) | info[100];
        }
        else
        {
            sectors = info[61];
            sectors = (sectors << 16) | info[60];
        }
    }

    // Store information
    ret->revision = revision;
    ret->flags = flags;
    ret->sectors = sectors;
    ret->pss = pss;
    ret->lss = lss;
    ret->atapi = atapi;
}

void libata_print(ata_info_t *info, const char *name, int bus_id, int dev_id)
{
    uint64_t size;
    const char *suffix;
    char buf[32];

    size = (info->sectors * info->lss);
    sprintf(buf, "%s%d.%d", name, bus_id, dev_id);

    kp_info(name, "%s: %s drive (revision %d)", buf, (info->atapi ? "ATAPI" : "ATA"), info->revision);
    kp_info(name, "%s: %s, %s, %s", buf, info->model, info->serial, info->firmware);
    kp_info(name, "%s: sector size: %d physical, %d logical", buf, info->pss, info->lss);

    if(size == 0)
    {
        return;
    }

    if(size > (1 << 30))
    {
        size /= (1 << 30);
        suffix = "GiB";
    }
    else
    {
        size /= (1 << 20);
        suffix = "MiB";
    }

    kp_info(name, "%s: %lu sectors, %lu %s", buf, info->sectors, size, suffix);
}

static void libata_worker(ata_worker_t *wk)
{
    ata_queue_t *item;
    int status;

    while(1)
    {
        item = list_pop(&wk->queue);
        if(item)
        {
            if(item->write)
            {
                status = wk->write(item->dev, item->lba, item->count, item->buf);
            }
            else
            {
                status = wk->read(item->dev, item->lba, item->count, item->buf);
            }

            item->status = status;
            thread_unblock(item->thread);
        }
        else
        {
            thread_block(0);
        }
    }

    thread_exit();
}

int libata_queue(ata_worker_t *wk, void *dev, int write, uint64_t lba, uint32_t count, void *buf)
{
    ata_queue_t item;
    int status;

    // Fill variables
    item.dev = dev;
    item.buf = buf;
    item.write = write;
    item.count = count;
    item.lba = lba;
    item.thread = thread_handle();

    // Append item to queue
    list_append(&wk->queue, &item);
    thread_unblock(wk->thread);

    // Wait until complete
    thread_block(0);
    status = item.status;

    // Return status
    return status;
}

int libata_start_worker(ata_worker_t *wk, const char *name)
{
    list_init(&wk->queue, offsetof(ata_queue_t, link));

    wk->thread = kthreads_create(name, libata_worker, wk, TPR_SRT);
    if(wk->thread == 0)
    {
        return -ENOMEM;
    }
    kthreads_run(wk->thread);

    return 0;
}
