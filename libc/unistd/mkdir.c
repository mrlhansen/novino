#include <novino/syscalls.h>
#include <errno.h>

int mkdir(const char *path, int mode)
{
    int status;

    status = sys_mkdir(path, mode);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
