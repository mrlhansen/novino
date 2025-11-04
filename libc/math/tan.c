#include <math.h>

double tan(double x)
{
    asm volatile("fptan; fstp %%st" : "+t"(x));
    return x;
}
