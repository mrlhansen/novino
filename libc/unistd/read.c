#include <novino/syscalls.h>
#include <errno.h>

ssize_t read(int fd, void *buf, size_t size)
{
    ssize_t status;

    status = sys_read(fd, size, buf);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return status;
}
