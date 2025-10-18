#include <stdio.h>

int snprintf(char *str, size_t n, const char *fmt, ...)
{
    int num;
    va_list args;
    va_start(args, fmt);
    num = vsnprintf(str, n, fmt, args);
    va_end(args);
    return num;
}
