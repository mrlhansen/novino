#include <stdlib.h>
#include <ctype.h>

long atol(const char *s)
{
    long res = 0;
    int sign = 1;
    int c = *s++;

    while(isspace(c))
    {
        c = *s++;
    }

    if(c == '-')
    {
        sign = -1;
        c = *s++;
    }
    else if(c == '+')
    {
        c = *s++;
    }

    while(isdigit(c))
    {
        res = res*10 + (c - '0');
        c = *s++;
    }

    return sign*res;
}
