#include <_syscall.h>
#include <_stdio.h>

long ftell(FILE *fp)
{
    long pos, diff;

    if(fp->flags & F_TEXT)
    {
        return 0;
    }

    pos = sys_seek(fp->fd, 0, SEEK_CUR);
    if(pos < 0)
    {
        errno = -pos;
        return -1L;
    }

    if(fp->b.mode == _IONBF)
    {
        return pos;
    }

    if(fp->mode == F_READ)
    {
        diff = (long)(fp->b.end - fp->b.pos);
        pos -= diff;
    }
    else if(fp->mode == F_WRITE)
    {
        diff = (long)(fp->b.pos - fp->b.ptr);
        pos += diff;
    }

    return pos;
}
