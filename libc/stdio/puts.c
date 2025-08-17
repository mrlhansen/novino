#include <string.h>
#include <stdio.h>

int puts(const char *str)
{
    int len, ch;

    len = fputs(str, stdout);
    if(len < 0)
    {
        return len;
    }

    ch = fputc('\n', stdout);
    if(ch < 0)
    {
        return ch;
    }

    return len + 1;
}
