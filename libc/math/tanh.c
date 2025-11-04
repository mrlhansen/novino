#include <math.h>

double tanh(double x)
{
    x = exp(2.0*x);
    return (x-1.0)/(x+1.0);
}
