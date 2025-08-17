#include <stdio.h>

int sscanf(const char *str, const char *fmt, ...)
{
    int nargs;
    va_list args;
    va_start(args, fmt);
    nargs = vsscanf(str, fmt, args);
    va_end(args);
    return nargs;
}
