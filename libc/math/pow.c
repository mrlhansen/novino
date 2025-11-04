#include <stddef.h>
#include <math.h>

// Rule 1: a^(-x) = 1/(a^x)
// Rule 2: a^x = 2^(log2(a)*x)
// Rule 3: a^(x+y) = a^x * a^y

// fyl2x calculates y*log2(x) with y=st(1) and x=st(0)
// f2xm1 calculates 2^x-1 for x=[-1,1]
// fscale calculates y*2^x where y=st(0) and x=RoundTowardZero[st(1)]

double pow(double base, double exp)
{
    int neg = 0;

    if(exp == 0.0)
    {
        return 1.0;
    }

    if(exp == 1.0)
    {
        return base;
    }

    if(exp < 0)
    {
        neg = 1;
        exp = -exp;
    }

    asm volatile(
        "fyl2x\n"
        "fst %%st(1)\n"
        "frndint\n"
        "fxch\n"
        "fsub %%st(1)\n"
        "f2xm1\n"
        "fld1\n"
        "fadd %%st(1)\n"
        "fld %%st(2)\n"
        "fxch\n"
        "fscale\n"
        "cmp $0, %%eax\n"
        "je .end\n"
        "fld1\n"
        "fdiv %%st(1)\n"
        ".end:\n"
        : "=t"(base)
        : "0"(base), "u"(exp), "a"(neg)
    );

    return base;
}
