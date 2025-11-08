#include <math.h>

// TODO: Argument reduction is needed if x is large, otherwise result is inaccurate

double sin(double x)
{
    asm volatile("fsin" : "+t"(x));
    return x;
}
