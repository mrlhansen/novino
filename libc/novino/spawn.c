#include <novino/syscalls.h>
#include <novino/spawn.h>
#include <_stdlib.h>
#include <_stdio.h>

pid_t spawnvef(const char *pathname, char *const argv[], char *const envp[], FILE *stdin, FILE *stdout)
{
    long status;

    status = sys_spawnve(pathname, argv, envp, stdin->fd, stdout->fd);
    if(status < 0)
    {
        errno = -status;
    }

    return status;
}

pid_t spawnve(const char *pathname, char *const argv[], char *const envp[])
{
    return spawnvef(pathname, argv, envp, stdin, stdout);
}

pid_t spawnv(const char *pathname, char *const argv[])
{
    return spawnvef(pathname, argv, environ, stdin, stdout);
}

pid_t wait(pid_t pid, int *status)
{
    pid = sys_wait(pid, status);
    if(pid < 0)
    {
        errno = -pid;
        return -1;
    }
    return pid;
}
