#include <math.h>

// Rule: log10(x) = log10(2)*log2(x)

// fldlg2 pushes log10(2) onto the stack
// fxch swaps st(0) and st(1)
// fyl2x calculates y*log2(x) with y=st(1) and x=st(0)

double log10(double x)
{
    if(x <= 0.0)
    {
        return -NAN;
    }
    asm volatile("fldlg2; fxch; fyl2x" : "+t"(x));
    return x;
}
