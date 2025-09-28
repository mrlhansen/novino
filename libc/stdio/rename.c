#include <_syscall.h>
#include <errno.h>

int rename(const char *oldpath, const char *newpath)
{
    int status;

    status = sys_rename(oldpath, newpath);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
