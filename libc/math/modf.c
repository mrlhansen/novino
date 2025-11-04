#include <math.h>

double modf(double x, double *iptr)
{
    double y;

    if(x == 0.0)
    {
        // preserve sign
        y = x;
    }
    else if(x < 0.0)
    {
        y = ceil(x);
        x = x - y;
    }
    else
    {
        y = floor(x);
        x = x - y;
    }

    *iptr = y;
    return x;
}
