#include <_stdio.h>

int fileno(FILE *fp)
{
    return fp->fd;
}
