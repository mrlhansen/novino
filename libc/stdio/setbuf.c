#include <stdio.h>

void setbuf(FILE *fp, char *buf)
{
    if(buf == NULL)
    {
        setvbuf(fp, NULL, _IONBF, 0);
    }
    else
    {
        setvbuf(fp, buf, _IOFBF, BUFSIZ);
    }
}
