#include <kernel/errno.h>
#include <_syscall.h>
#include <errno.h>

int remove(const char *path)
{
    int status;

    status = sys_remove(path);
    if(!status)
    {
        return 0;
    }

    if(status != -EISDIR)
    {
        errno = -status;
        return -1;
    }

    status = sys_rmdir(path);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
