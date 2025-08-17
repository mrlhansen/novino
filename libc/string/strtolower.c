#include <string.h>
#include <ctype.h> 

char *strtolower(char *str)
{
    char *ptr = str;
    unsigned char val;

    while(*ptr)
    {
        val = (unsigned char)*ptr;
        val = tolower(val);
        *ptr++ = val;
    }

    return str;
}
