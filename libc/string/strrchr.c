#include <string.h>

char *strrchr(const char *str, int val)
{
    char *rtnval = NULL;

    do
    {
        if(*str == (char)val)
        {
            rtnval = (char*)str;
        }
    } while(*str++);

    return rtnval;
}
