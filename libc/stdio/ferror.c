#include <_stdio.h>

int ferror(FILE *fp)
{
    return (fp->flags & F_ERR) ? -1 : 0;
}
