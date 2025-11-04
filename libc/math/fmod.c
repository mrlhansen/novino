#include <math.h>

double fmod(double x, double y)
{
    asm volatile(
        ".loop:\n"
        "fprem\n"
        "fstsw %%ax\n"
        "sahf\n"
        "jpe .loop\n"
        : "=t"(x)
        : "0"(x), "u"(y)
        : "ax", "cc"
    );
    return x;
}
