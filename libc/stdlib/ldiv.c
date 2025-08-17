#include <stdlib.h>

ldiv_t ldiv(long n, long d)
{
    ldiv_t r;

    r.quot = n / d;
    r.rem = n % d;

    if(n >= 0 && r.rem < 0)
    {
        r.quot++;
        r.rem -= d;
    }

    return r;
}
