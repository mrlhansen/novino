#include <stdio.h>

int scanf(const char *fmt, ...)
{
    int nargs;
    va_list args;
    va_start(args, fmt);
    nargs = vfscanf(stdin, fmt, args);
    va_end(args);
    return nargs;
}
