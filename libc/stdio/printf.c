#include <stdio.h>

int printf(const char *fmt, ...)
{
    size_t len;
    va_list args;
    va_start(args, fmt);
    len = vfprintf(stdout, fmt, args);
    va_end(args);
    return len;
}
