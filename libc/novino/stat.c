#include <kernel/vfs/types.h>
#include <novino/syscalls.h>
#include <nonstd.h>
#include <errno.h>

int stat(const char *path, struct stat *buf)
{
    stat_t stat;
    int status;

    status = sys_stat(path, &stat);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    if(buf)
    {
        buf->st_dev = 0;
        buf->st_ino = stat.ino;
        buf->st_mode = stat.mode;
        buf->st_nlink = 0;
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
