#include <_syscall.h>
#include <errno.h>

ssize_t write(int fd, void *buf, size_t size)
{
    ssize_t status;

    status = sys_write(fd, size, buf);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return status;
}
