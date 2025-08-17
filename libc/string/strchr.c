#include <string.h>

char *strchr(const char *str, int val)
{
    do
    {
        if(*str == (char)val)
        {
            return (char*)str;
        }
    } while(*str++);

    return NULL;
}
