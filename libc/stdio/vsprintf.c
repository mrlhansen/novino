#include <_stdio.h>
#include <limits.h>

int vsprintf(char *str, const char *fmt, va_list args)
{
    return vsnprintf(str, INT_MAX, fmt, args);
}
