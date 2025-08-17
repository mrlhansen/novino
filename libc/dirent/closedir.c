#include <_syscall.h>
#include <_stdio.h>
#include <dirent.h>

int closedir(DIR *dp)
{
    int status;

    if(dp == NULL)
    {
        // set errno
        return -1;
    }

    status = sys_close(dp->fd);
    __libc_fd_free(dp);

    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
