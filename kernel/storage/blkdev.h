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
    size_t bps;     // Bytes per sector
    size_t sectors; // Total number of sectors
    size_t offset;  // Partition sector offset
    size_t size;    // Partition sector size
    size_t flags;   // Block device flags
    lock_t lock;    // Lock for exclusive access
    bool dynsz;     // Dynamic size
} blkdev_t;

int blkdev_alloc(devfs_t *dev, size_t offset, size_t size, bool partitionable);
int blkdev_free(devfs_t *dev);

int blkdev_open(devfs_t *dev);
void blkdev_close(devfs_t *dev);

int blkdev_read(devfs_t *dev, size_t offset, size_t count, void *data);
int blkdev_write(devfs_t *dev, size_t offset, size_t count, void *data);
