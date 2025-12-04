#include <_stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int putenv(char *string)
{
    int pos;

    if(strchr(string, '=') == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    pos = __libc_getenv(string);
    return __libc_putenv(string, 0, pos);
}
