#include <math.h>

double atan2(double y, double x)
{
    if(x == 0.0)
    {
        if(y > 0.0)
        {
            return M_PI / 2.0;
        }
        if(y < 0.0)
        {
            return -M_PI / 2.0;
        }
        return NAN;
    }

    asm volatile("fpatan" : "=t"(x) : "0"(x), "u"(y));
    return x;
}
