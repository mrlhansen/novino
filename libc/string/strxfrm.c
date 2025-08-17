#include <string.h>

size_t strxfrm(char *dest, const char *src, size_t n)
{
    size_t n2 = strlen(src);

    if(n > n2)
    {
        strcpy(dest, src);
    }

    return n2;
}
