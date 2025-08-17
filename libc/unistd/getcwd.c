#include <_syscall.h>
#include <unistd.h>
#include <errno.h>

char *getcwd(char *buf, size_t size)
{
    int status;

    status = sys_getcwd(buf, size);
    if(status < 0)
    {
        errno = -status;
        return NULL;
    }

    return buf;
}
