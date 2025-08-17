#include <string.h>

void *memset(void *dest, int val, size_t len)
{
    unsigned char *dp = dest;

    while(len--)
    {
        *dp++ = (unsigned char)val;
    }

    return dest;
} 
