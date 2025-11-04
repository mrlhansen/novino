#include <math.h>

double sin(double x)
{
    asm volatile("fsin" : "+t"(x));
    return x;
}
