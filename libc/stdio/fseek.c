#include <_syscall.h>
#include <_stdio.h>

int fseek(FILE *fp, long offset, int origin)
{
    long status;

    if((origin != SEEK_CUR) && (origin != SEEK_SET) && (origin != SEEK_END))
    {
        // set errno
        return -1;
    }

    if(fp->flags & F_TEXT)
    {
        return offset ? -1 : 0;
    }

    fflush(fp);

    status = sys_seek(fp->fd, offset, origin);
    if(status < 0)
    {
        errno = -status;
        return -1;
    }

    fp->flags &= ~F_EOF;

    return 0;
}
