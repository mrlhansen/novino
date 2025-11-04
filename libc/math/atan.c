#include <math.h>

// fpatan calculates atan(y/x) with y=st(1) and x=st(0)

double atan(double x)
{
    asm volatile("fld1; fpatan" : "+t"(x));
    return x;
}
