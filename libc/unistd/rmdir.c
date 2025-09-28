#include <_syscall.h>
#include <errno.h>

int rmdir(const char *path)
{
    int status;

    status = sys_rmdir(path);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
