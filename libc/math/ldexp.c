#include <math.h>

double ldexp(double x, int y)
{
    double dy = y;
    asm volatile("fscale" : "=t"(x) : "0"(x), "u"(dy));
    return x;
}
