#include <math.h>

// Rule: ln(x) = ln(2)*log2(x)

// fldln2 pushes ln(2) onto the stack
// fxch swaps st(0) and st(1)
// fyl2x calculates y*log2(x) with y=st(1) and x=st(0)

double log(double x)
{
    if(x <= 0.0)
    {
        return NAN;
    }
    asm volatile("fldln2; fxch; fyl2x" : "+t"(x));
    return x;
}
