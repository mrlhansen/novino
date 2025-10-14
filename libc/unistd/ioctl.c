#include <_syscall.h>
#include <stdarg.h>
#include <errno.h>

int ioctl(int fd, size_t op, ...)
{
    int status;
    size_t arg;
    va_list va;

    va_start(va, op);
    arg = va_arg(va, size_t);
    va_end(va);

    status = sys_ioctl(fd, op, arg);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
