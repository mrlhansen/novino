#include <math.h>

// Rule: asin(x) = atan(x/sqrt(1-x^2))

double asin(double x)
{
    if(x < -1.0)
    {
        return NAN;
    }

    if(x > 1.0)
    {
        return NAN;
    }

    if(x == 1.0)
    {
        return M_PI / 2.0;
    }

    if(x == -1.0)
    {
        return -M_PI / 2.0;
    }

    if(x == 0.0)
    {
        return 0.0;
    }

    asm volatile(
        "fst %%st(1)\n"
        "fmul %%st\n"
        "fld1\n"
        "fsub %%st(1)\n"
        "fsqrt\n"
        "fld %%st(2)\n"
        "fxch\n"
        "fpatan\n"
        : "=t"(x)
        : "0" (x)
    );

    return x;
}
