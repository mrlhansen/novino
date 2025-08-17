#include <string.h>

char* strrev(char *str)
{
    char *s = str;
    char *e = str;
    char ch;

    while(*e) e++;
    e--;

    while(s < e)
    {
        ch = *s;
        *s++ = *e;
        *e-- = ch;
    }

    return str;
}
