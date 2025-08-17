#include <_stdio.h>

int feof(FILE *fp)
{
    return (fp->flags & F_EOF) ? -1 : 0;
}
