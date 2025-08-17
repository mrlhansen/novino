#include <string.h>

void *memchr(const void *ptr, int val, size_t len)
{
    unsigned char *p = (unsigned char*)ptr;

    while(len--)
    {
        if(*p == (unsigned char)val)
        {
            return p;
        }
        else
        {
            p++;
        }
    }

    return 0;
}
