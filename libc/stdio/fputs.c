#include <string.h>
#include <stdio.h>

int fputs(const char *str, FILE *fp)
{
    size_t count;
    size_t len;

    len = strlen(str);
    count = fwrite(str, 1, len, fp);
    if(count != len)
    {
        return EOF;
    }

    return len;
}
