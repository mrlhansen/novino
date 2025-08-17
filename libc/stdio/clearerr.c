#include <_stdio.h>

void clearerr(FILE *fp)
{
    fp->flags &= ~(F_EOF | F_ERR);
}
