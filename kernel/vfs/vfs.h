#pragma once

#include <kernel/vfs/types.h>

dentry_t *dcache_lookup(dentry_t*, const char*);
dentry_t *dcache_append(dentry_t *parent, const char *name, inode_t *inode, int invalid);

// TODO: should all these return ssize_t and same for the corresponding vfs_ops ?
int vfs_open(const char *pathname, int flags);
int vfs_close(int fd);
int vfs_read(int fd, size_t size, void *buf);
int vfs_write(int fd, size_t size, void *buf);
int vfs_seek(int fd, long offset, int origin);
int vfs_ioctl(int fd, size_t cmd, size_t val);
int vfs_stat(const char *pathname, stat_t *stat);
int vfs_fstat(int fd, stat_t *stat);
int vfs_chdir(const char *pathname);
int vfs_getcwd(char *pathname, int size);
int vfs_readdir(int fd, size_t count, dirent_t *dirent);

void vfs_filler(void*, const char*, inode_t*);
int vfs_mount(const char*, const char*, const char*);

int vfs_register(const char*, vfs_ops_t*);
void vfs_init();
