#include <string.h>

void *memmove(void *dest, const void *src, size_t len)
{
    unsigned char *p1 = dest;
    const unsigned char *p2 = src;

    if((p2 < p1) && (p1 < p2 + len))
    {
        p1 = p1 + len;
        p2 = p2 + len;

        while(len--)
        {
            *--p1 = *--p2;
        }
    }
    else
    {
        while(len--)
        {
            *p1++ = *p2++;
        }
    }

    return dest;
}
