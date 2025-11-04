#include <math.h>

double cos(double x)
{
    asm volatile("fcos" : "+t"(x));
    return x;
}
