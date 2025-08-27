#pragma once

#include <kernel/vfs/types.h>

devfs_t *devfs_stream_register(devfs_t *parent, const char *name, devfs_ops_t *ops, void *data, stat_t *stat);
devfs_t *devfs_block_register(devfs_t *parent, const char *name, devfs_blk_t *ops, void *data, stat_t *stat);
devfs_t *devfs_mkdir(devfs_t *parent, const char *name);

void devfs_init();
