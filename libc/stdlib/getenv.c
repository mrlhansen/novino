#include <_stdlib.h>
#include <string.h>

char *getenv(const char *name)
{
    char *delim;
    int pos;

    pos = __libc_getenv(name);
    if(pos < 0)
    {
        return NULL;
    }

    delim = strchr(environ[pos], '=');
    if(delim)
    {
        return delim + 1;
    }

    return environ[pos];
}
