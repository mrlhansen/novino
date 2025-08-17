#include <stdio.h>

int fprintf(FILE *fp, const char *fmt, ...)
{
    int num;
    va_list args;
    va_start(args, fmt);
    num = vfprintf(fp, fmt, args);
    va_end(args);
    return num;
}
