#include <novino/syscalls.h>
#include <unistd.h>
#include <errno.h>

int close(int fd)
{
    int status;

    status = sys_close(fd);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
