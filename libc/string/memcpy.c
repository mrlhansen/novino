#include <string.h>

void *memcpy(void *dest, const void *src, size_t len)
{
    const char *sp = src;
    char *dp = dest;

    while(len--)
    {
        *dp++ = *sp++;
    }

    return dest;
}
