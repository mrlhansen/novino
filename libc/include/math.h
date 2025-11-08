#pragma once

#define M_E  2.71828182845904523536L
#define M_PI 3.14159265358979323846L

#define NAN      __builtin_nanf("")
#define INFINITY __builtin_inff()

double cos(double x);
double sin(double x);
double tan(double x);

double acos(double x);
double asin(double x);
double atan(double x);
double atan2(double x, double y);

double cosh(double x);
double sinh(double x);
double tanh(double x);

double sqrt(double x);
double pow(double x, double y);
double exp(double x);
double ldexp(double x, int y);
double frexp(double x, int *y);
double log10(double x);
double log(double x);

double modf(double x, double *iptr);
double fmod(double x, double y);
double ceil(double x);
double floor(double x);
double fabs(double x);
