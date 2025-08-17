#include <stdio.h>

int fputc(int ch, FILE *fp)
{
    unsigned char val = ch;
    if(fwrite(&val, 1, 1, fp) == 1)
    {
        return val;
    }
    return EOF;
}
