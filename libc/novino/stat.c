#include <kernel/vfs/types.h>
#include <novino/syscalls.h>
#include <novino/stat.h>
#include <errno.h>

int stat(const char *path, struct stat *buf)
{
    stat_t stat;
    size_t mode;
    int status;

    status = sys_stat(path, &stat);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    if(buf)
    {
        mode = stat.flags;
        mode = (mode << 12) | stat.mode;

        buf->st_dev = 0;
        buf->st_ino = stat.ino;
        buf->st_mode = mode;
        buf->st_nlink = stat.links;
        buf->st_uid = stat.uid;
        buf->st_gid = stat.gid;
        buf->st_rdev = 0;
        buf->st_size = stat.size;
        buf->st_atime = stat.atime;
        buf->st_mtime = stat.mtime;
        buf->st_ctime = stat.ctime;
        buf->st_blksize = stat.blksz;
        buf->st_blocks = stat.blocks;
    }

    return 0;
}
