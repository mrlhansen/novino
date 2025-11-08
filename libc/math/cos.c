#include <math.h>

// TODO: Argument reduction is needed if x is large, otherwise result is inaccurate

double cos(double x)
{
    asm volatile("fcos" : "+t"(x));
    return x;
}
