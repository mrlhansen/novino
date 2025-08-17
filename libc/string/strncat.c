#include <string.h>

char *strncat(char *dest, const char *src, size_t len)
{
    char *dp = dest;

    while(*dest)
    {
        dest++;
    }

    while(*src && len)
    {
        *dest++ = *src++;
        len--;
    }

    *dest = '\0';
    return dp;
}
