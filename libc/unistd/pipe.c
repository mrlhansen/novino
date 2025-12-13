#include <novino/syscalls.h>
#include <errno.h>

int pipe(int *fd)
{
    int status;

    status = sys_mkpipe(fd);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
