#include <string.h>
#include <ctype.h>

int strncasecmp(const char *s1, const char *s2, int len)
{
    const unsigned char *p1 = (const void*)s1;
    const unsigned char *p2 = (const void*)s2;
    unsigned char a, b;

    a = tolower(*p1++);
    b = tolower(*p2++);

    while(len && a == b)
    {
        if(a == '\0')
        {
            return 0;
        }
        a = tolower(*p1++);
        b = tolower(*p2++);
        len--;
    }

    if(!len)
    {
        return 0;
    }

    return (a < b) ? -1 : 1;
}
