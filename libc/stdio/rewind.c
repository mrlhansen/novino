#include <_stdio.h>

void rewind(FILE *fp)
{
    fseek(fp, 0, SEEK_SET);
    fp->flags &= ~F_ERR;
}
