#include <string.h>
#include <stdlib.h>

char *strdup(const char *str)
{
    char *dst;
    int len;

    len = strlen(str);
    dst = malloc(len+1);
    if(!dst)
    {
        return NULL;
    }

    strcpy(dst, str);
    return dst;
}
