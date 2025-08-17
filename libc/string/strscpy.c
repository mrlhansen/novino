#include <string.h>

ssize_t strscpy(char *dest, const char *src, size_t len)
{
    ssize_t rlen = 0;

    // No space
    if(len == 0)
    {
        return 0;
    }

    // Copy as much as possible
    while(len && *src)
    {
        *dest++ = *src++;
        rlen++;
        len--;
    }

    // Null terminate
    if(len)
    {
        dest[0] = '\0';
    }
    else
    {
        dest[-1] = '\0';
        rlen--;
    }

    // Destination too small
    if(*src)
    {
        rlen = -1; // -E2BIG
    }

    return rlen;
}
