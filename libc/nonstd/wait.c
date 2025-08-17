#include <_syscall.h>
#include <nonstd.h>

long wait(long pid, int *status)
{
    return sys_wait(pid, status);
}
