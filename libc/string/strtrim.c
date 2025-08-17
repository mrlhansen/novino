#include <string.h>
#include <ctype.h>

char *strtrim(char *dest, const char *src)
{
    const char *start,  *end;
    char *dp = dest;
    size_t len;

    len = strlen(src);
    start = src;
    end = src + len - 1;

    if(len == 0)
    {
        dest[0] = '\0';
        return dp;
    }

    while(isspace((unsigned char)*start))
    {
        start++;
    }

    while(isspace((unsigned char)*end))
    {
        end--;
    }

    while(start <= end)
    {
        *dest++ = *start++;
    }

    *dest = '\0';
    return dp;
}
