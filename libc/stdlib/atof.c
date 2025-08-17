#include <stdlib.h>
#include <ctype.h>

double atof(const char *s)
{
    double a = 0.0;
    double as = 1.0;
    int e = 0;
    int c = *s++;

    while(isspace(c))
    {
        c = *s++;
    }

    if(c == '-')
    {
        as = -1.0;
        c = *s++;
    }
    else if(c == '+')
    {
        c = *s++;
    }

    while(isdigit(c))
    {
        a = a*10.0 + (c - '0');
        c = *s++;
    }

    if(c == '.')
    {
        c = *s++;
        while(isdigit(c))
        {
            a = a*10.0 + (c - '0');
            e = e-1;
            c = *s++;
        }
    }

    if(c == 'e' || c == 'E')
    {
        int sign = 1;
        int i = 0;
        c = *s++;

        if(c == '+')
        {
            c = *s++;
        }
        else if(c == '-')
        {
            c = *s++;
            sign = -1;
        }

        while(isdigit(c))
        {
            i = i*10 + (c - '0');
            c = *s++;
        }

        e += i*sign;
    }

    while(e > 0)
    {
        a *= 10.0;
        e--;
    }

    while(e < 0)
    {
        a *= 0.1;
        e++;
    }

    return as*a;
}
