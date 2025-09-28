#pragma once

#include <kernel/sched/process.h>
#include <kernel/vfs/types.h>

void dcache_purge(dentry_t *root);
void dcache_delete(dentry_t *item);

void dcache_mark_positive(dentry_t *item);
void dcache_mark_negative(dentry_t *item);

dentry_t *dcache_lookup(dentry_t*, const char *name);
dentry_t *dcache_append(dentry_t *parent, const char *name, inode_t *inode);

void vfs_proc_init(process_t *pr, dentry_t *cwd);
void vfs_proc_fini(process_t *pr);

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

int vfs_put_dirent(void*, const char*, inode_t*);
int vfs_readdir(int fd, size_t count, dirent_t *dirent);

int vfs_create(const char *pathname, int mode);
int vfs_remove(const char *pathname);
int vfs_rename(const char *oldpath, const char *newpath);

int vfs_mkdir(const char *pathname, int mode);
int vfs_rmdir(const char *pathname);

int vfs_register(const char *fstype, vfs_ops_t *ops);
int vfs_mount(const char *source, const char *fstype, const char *target);
int vfs_umount(const char *target);

void vfs_init();
