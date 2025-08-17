#include <string.h>

char *strcpy(char *dest, const char *src)
{
    char *dp = dest;

    while(*src)
    {
        *dest++ = *src++;
    }

    *dest = '\0';
    return dp;
}
