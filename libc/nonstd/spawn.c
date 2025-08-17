#include <_syscall.h>
#include <_stdlib.h>
#include <_stdio.h>
#include <unistd.h>

long spawnvef(const char *pathname, char *const argv[], char *const envp[], FILE *stdin, FILE *stdout)
{
    long status;

    status = sys_spawnve(pathname, argv, envp, stdin->fd, stdout->fd);
    if(status < 0)
    {
        errno = -status;
    }

    return status;
}

long spawnve(const char *pathname, char *const argv[], char *const envp[])
{
    return spawnvef(pathname, argv, envp, stdin, stdout);
}

long spawnv(const char *pathname, char *const argv[])
{
    return spawnvef(pathname, argv, environ, stdin, stdout);
}
