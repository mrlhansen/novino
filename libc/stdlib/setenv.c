#include <_stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int setenv(const char *name, const char *value, int overwrite)
{
    char *str;
    int size;
    int pos;

    if(name == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if(*name == '\0')
    {
        errno = EINVAL;
        return -1;
    }

    if(strchr(name, '=') != NULL)
    {
        errno = EINVAL;
        return -1;
    }

    pos = __libc_getenv(name);
    if(!overwrite && pos >= 0)
    {
        return 0;
    }

    size = 2 + strlen(name) + strlen(value);
    str = malloc(size);
    if(!str)
    {
        errno = ENOMEM;
        return -1;
    }
    sprintf(str, "%s=%s", name, value);

    return __libc_putenv(str, 1, pos);
}
