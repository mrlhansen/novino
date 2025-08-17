#include <string.h>

void *memsetw(void *dest, int val, size_t len)
{
    unsigned short *dp = dest;

    while(len--)
    {
        *dp++ = (unsigned short)val;
    }

    return dest;
} 
