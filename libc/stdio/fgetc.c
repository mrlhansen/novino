#include <_stdio.h>

int fgetc(FILE *fp)
{
    unsigned char ch;
    if(fread(&ch, 1, 1, fp) == 1)
    {
        return ch;
    }
    return EOF;
}
