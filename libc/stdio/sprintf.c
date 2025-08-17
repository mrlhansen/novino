#include <stdio.h>

int sprintf(char *str, const char *fmt, ...)
{
    int num;
    va_list args;
    va_start(args, fmt);
    num = vsprintf(str, fmt, args);
    va_end(args);
    return num;
}
