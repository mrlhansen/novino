#include <stdio.h>

int fgetpos(FILE *fp, fpos_t *pos)
{
    long off = ftell(fp);
    if(off < 0) return -1;
    *pos = off;
    return 0;
}
