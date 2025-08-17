#include <string.h>

char *strncpy(char *dest, const char *src, size_t len)
{
    char *dp = dest;

    while(len && *src)
    {
        *dest++ = *src++;
        len--;
    }

    while(len--)
    {
        *dest++ = '\0';
    }

    return dp;
}
