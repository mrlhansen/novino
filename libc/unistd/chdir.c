#include <_syscall.h>
#include <unistd.h>
#include <errno.h>

int chdir(const char *path)
{
    int status;

    status = sys_chdir(path);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    return 0;
}
