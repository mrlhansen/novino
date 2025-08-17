#include <stdio.h>

int fscanf(FILE *fp, const char *fmt, ...)
{
    int num;
    va_list args;
    va_start(args, fmt);
    num = vfscanf(fp, fmt, args);
    va_end(args);
    return num;
}
