#include <_stdlib.h>
#include <string.h>

int unsetenv(const char *name)
{
    if(name == NULL)
    {
        // errno = -EINVAL;
        return -1;
    }

    if(*name == '\0')
    {
        // errno = -EINVAL;
        return -1;
    }

    if(strchr(name, '=') != NULL)
    {
        // errno = -EINVAL;
        return -1;
    }

    return __libc_unsetenv(name);
}
