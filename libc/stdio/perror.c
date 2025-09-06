#include <string.h>
#include <stdio.h>
#include <errno.h>

void perror(const char *str)
{
    if(str)
    {
        printf("%s: %s\n", str, strerror(errno));
    }
    else
    {
        printf("%s\n", strerror(errno));
    }
}
