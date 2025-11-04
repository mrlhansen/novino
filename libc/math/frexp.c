#include <math.h>

double frexp(double x, int *y)
{
    double dy;
    asm volatile("fxtract" : "=t"(x), "=u"(dy) : "0"(x));
    *y = 1.0+dy;
    return x/2.0;
}
