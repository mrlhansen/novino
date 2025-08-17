#include <string.h>

char *strstr(const char *s1, const char *s2)
{
    size_t n = strlen(s2);

    while(*s1)
    {
        if(memcmp(s1++, s2, n) == 0)
        {
            return (char*)(s1-1);
        }
    }

    return 0;
}
