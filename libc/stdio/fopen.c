#include <kernel/vfs/types.h>
#include <novino/syscalls.h>
#include <_stdio.h>
#include <stdlib.h>

static int parse_mode(const char *mode, int *kf, int *uf)
{
    int r = 0;
    int w = 0;
    int a = 0;
    int p = 0;

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
        errno = EINVAL;
        return -1;
    }

    if(r && p)
    {
        *kf = O_READ | O_WRITE;
        *uf = F_READ | F_WRITE;
    }
    else if(r)
    {
        *kf = O_READ;
        *uf = F_READ;
    }
    else if(w && p)
    {
        *kf = O_READ | O_WRITE | O_TRUNC | O_CREATE;
        *uf = F_READ | F_WRITE;
    }
    else if(w)
    {
        *kf = O_WRITE | O_TRUNC | O_CREATE;
        *uf = F_WRITE;
    }
    else if(a && p)
    {
        *kf = O_READ | O_WRITE | O_APPEND | O_CREATE;
        *uf = F_READ | F_WRITE | F_APPEND;
    }
    else if(a)
    {
        *kf = O_WRITE | O_APPEND | O_CREATE;
        *uf = F_WRITE | F_APPEND;
    }

    return 0;
}

FILE *fdopen(int fd, const char *mode)
{
    FILE *fp;
    int kflags;
    int uflags;

    if(parse_mode(mode, &kflags, &uflags) < 0)
    {
        return NULL;
    }

    fp = __libc_fd_alloc(fd);
    if(!fp)
    {
        return NULL;
    }
    fp->flags = uflags;

    return fp;
}

FILE *fopen(const char *filename, const char *mode)
{
    FILE *fp;
    int kflags;
    int uflags;
    int fd;

    if(parse_mode(mode, &kflags, &uflags) < 0)
    {
        return NULL;
    }

    fd = sys_open(filename, kflags);
    if(fd < 0)
    {
        errno = -fd;
        return NULL;
    }

    fp = __libc_fd_alloc(fd);
    if(!fp)
    {
        sys_close(fd);
        return NULL;
    }
    fp->flags = uflags;

    return fp;
}
