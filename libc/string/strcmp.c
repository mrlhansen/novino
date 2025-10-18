#include <string.h>

int strcmp(const char *s1, const char *s2)
{
    const unsigned char *p1 = (const void*)s1;
    const unsigned char *p2 = (const void*)s2;

    while(*p1 == *p2)
    {
        if(*p1 == '\0')
        {
            return 0;
        }

        p1++;
        p2++;
    }

    return (*p1 < *p2) ? -1 : 1;
}
