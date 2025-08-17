#include <_stdio.h>

int fflush(FILE *fp)
{
    long status;

    if(fp->mode == F_WRITE)
    {
        status = __libc_fp_flush(fp);
        if(status < 0)
        {
            return EOF;
        }
    }

    fp->mode = 0;
    fp->u.pos = fp->u.ptr;

    fp->b.pos = fp->b.ptr;
    fp->b.end = fp->b.ptr;

    return 0;
}
