#include <kernel/vfs/types.h>
#include <_syscall.h>
#include <_stdio.h>
#include <stdlib.h>

FILE *fopen(const char *filename, const char *mode)
{
    int kflags = 0;
    int uflags = 0;
    int r = 0;
    int w = 0;
    int a = 0;
    int p = 0;
    int fd;
    FILE *fp;

    while(*mode)
    {
        switch(*mode)
        {
            case 'r':
                r = 1;
                break;
            case 'w':
                w = 1;
                break;
            case 'a':
                a = 1;
                break;
            case '+':
                p = 1;
                break;
        }
        mode++;
    }

    if(r + w + a != 1)
    {
        // set errno
        return 0;
    }

    if(r && p)
    {
        kflags = O_READ | O_WRITE;
        uflags = F_READ | F_WRITE;
    }
    else if(r)
    {
        kflags = O_READ;
        uflags = F_READ;
    }
    else if(w && p)
    {
        kflags = O_READ | O_WRITE | O_TRUNC | O_CREATE;
        uflags = F_READ | F_WRITE;
    }
    else if(w)
    {
        kflags = O_WRITE | O_TRUNC | O_CREATE;
        uflags = F_WRITE;
    }
    else if(a && p)
    {
        kflags = O_READ | O_WRITE | O_APPEND | O_CREATE;
        uflags = F_READ | F_WRITE | F_APPEND;
    }
    else if(a)
    {
        kflags = O_WRITE | O_APPEND | O_CREATE;
        uflags = F_WRITE | F_APPEND;
    }

    fd = sys_open(filename, kflags);
    if(fd < 0)
    {
        errno = -fd;
        return NULL;
    }

    fp = __libc_fd_alloc(fd);
    if(fp == NULL)
    {
        sys_close(fd);
        return NULL;
    }
    fp->flags = uflags;

    return fp;
}
