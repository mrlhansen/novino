#include <string.h>

int strncmp(const char *s1, const char *s2, size_t len)
{
    const unsigned char *p1 = (const void*)s1;
    const unsigned char *p2 = (const void*)s2;

    while(len && *p1 == *p2)
    {
        if(*p1 == '\0')
        {
            return 0;
        }
        p1++;
        p2++;
        len--;
    }

    if(!len)
    {
        return 0;
    }

    return (*p1 < *p2) ? -1 : 1;
}
