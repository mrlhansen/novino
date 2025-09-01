#pragma once

#include <kernel/vfs/types.h>

// Block device flags
enum {
    BLKDEV_REMOVEABLE    = (1 << 0),
    BLKDEV_MEDIA_ABSENT  = (1 << 1),
    BLKDEV_MEDIA_CHANGED = (1 << 2),
};

// Block device descriptor
typedef struct blkdev {
    size_t bps;
    size_t sectors;
    size_t offset;
    size_t flags;
} blkdev_t;

int blkdev_open(devfs_t *dev);
void blkdev_close(devfs_t *dev);
int blkdev_read(devfs_t *dev, size_t offset, size_t count, void *data);
int blkdev_write(devfs_t *dev, size_t offset, size_t count, void *data);
