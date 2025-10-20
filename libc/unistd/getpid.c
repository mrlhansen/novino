#include <novino/syscalls.h>
#include <unistd.h>

pid_t getpid()
{
    return sys_getpid();
}
