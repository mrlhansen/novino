#include <_stdio.h>

int ungetc(int ch, FILE *fp)
{
    char *ungetmax;

    // check that stream is open for reading
    if((fp->flags & F_READ) == 0)
    {
        return EOF;
    }

    // cannot push EOF onto stream
    if(ch == EOF)
    {
        return EOF;
    }

    // check for available space
    ungetmax = fp->u.ptr + UNGETSIZ;
    if(fp->u.pos == ungetmax)
    {
        return EOF;
    }

    // push onto stream
    *fp->u.pos++ = ch;
    fp->flags &= ~F_EOF;

    return ch;
}
