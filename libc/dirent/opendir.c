#include <kernel/vfs/types.h>
#include <novino/syscalls.h>
#include <_stdio.h>
#include <dirent.h>

DIR *opendir(const char *dirname)
{
    DIR *dp;
    int fd;

    fd = sys_open(dirname, O_DIR);
    if(fd < 0)
    {
        errno = -fd;
        return NULL;
    }

    dp = __libc_fd_alloc(fd);
    if(dp == NULL)
    {
        sys_close(fd);
        return NULL;
    }
    dp->flags = F_DIR;

    return dp;
}
