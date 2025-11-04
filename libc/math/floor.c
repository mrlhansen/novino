#include <math.h>

double floor(double x)
{
    double y;
    asm volatile("frndint" : "=t"(y) : "0"(x));
    if(y > x)
    {
        y = y - 1.0;
    }
    return y;
}
