#include <math.h>

double sqrt(double x)
{
    if(x <= 0.0)
    {
        return NAN;
    }
    asm volatile("fsqrt" : "+t"(x));
    return x;
}
