#include <_stdio.h>

char *fgets(char *str, int size, FILE *fp)
{
    char *buf = str;
    int ch;

    if(size == 0)
    {
        return NULL;
    }

    if(fp->b.mode == _IOLBF)
    {
        size = fread(str, 1, size-1, fp);
        str[size] = '\0';
    }
    else
    {
        while(size > 1)
        {
            ch = fgetc(fp);
            if(ch == EOF)
            {
                break;
            }
            else if(ch == '\n')
            {
                size = 0;
            }
            *buf++ = ch;
            size--;
        }
        *buf = '\0';
    }

    if(fp->flags & (F_ERR | F_EOF))
    {
        return NULL;
    }

    return str;
}
