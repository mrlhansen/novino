#include <novino/syscalls.h>
#include <_stdio.h>
#include <stdlib.h>

int fclose(FILE *fp)
{
    int status;

    if(fp == NULL)
    {
        // set errno?
        return EOF;
    }

    fflush(fp);
    status = sys_close(fp->fd);
    __libc_fd_free(fp);

    if(status < 0)
    {
        errno = -status;
        return EOF;
    }

    return 0;
}
