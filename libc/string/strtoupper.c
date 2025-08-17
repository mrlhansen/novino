#include <string.h>
#include <ctype.h> 

char *strtoupper(char *str)
{
    char *ptr = str;
    unsigned char val;

    while(*ptr)
    {
        val = (unsigned char)*ptr;
        val = toupper(val);
        *ptr++ = val;
    }

    return str;
}
