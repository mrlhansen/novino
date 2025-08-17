#include <stdlib.h>

div_t div(int n, int d)
{
    div_t r;

    r.quot = n / d;
    r.rem = n % d;

    if(n >= 0 && r.rem < 0)
    {
        r.quot++;
        r.rem -= d;
    }

    return r;
}
