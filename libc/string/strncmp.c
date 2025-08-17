#include <string.h>

int strncmp(const char *str1, const char *str2, size_t len)
{
    while(len && *str1 && *str1 == *str2)
    {
        str1++;
        str2++;
        len--;
    }

    if(len == 0)
    {
        return 0;
    }
    else
    {
        return (*str1 < *str2) ? -1 : 1;
    }
}
