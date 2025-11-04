#include <math.h>

// See pow.c for explanations, only difference is the base

double exp(double x)
{
    double x;
    int neg = 0;

    if(x == 0.0)
    {
        return 1.0;
    }

    if(x == 1.0)
    {
        return M_E;
    }

    if(x < 0)
    {
        neg = 1;
        x = -x;
    }

    asm volatile(
        "fldl2e\n"
        "fmul %%st(1)\n"
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
        : "=t"(x)
        : "0"(x), "a"(neg)
    );

    return x;
}
