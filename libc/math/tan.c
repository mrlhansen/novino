#include <math.h>

// TODO: Argument reduction is needed if x is large, otherwise result is inaccurate

double tan(double x)
{
    asm volatile("fptan; fstp %%st" : "+t"(x));
    return x;
}
