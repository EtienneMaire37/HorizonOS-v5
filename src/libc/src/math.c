#include "../include/math.h"

double fabs(double x)
{
    return x < 0 ? -x : x;
}

double intpow(double a, long long b)
{
    if (b == 0)
        return 1;
    if (b < 0)
        return intpow(a, -b);

    double result = 1.;
    for (long long i = 0; i < b; i++)
        result *= a;
    return result;
}

#define k2  0.5
#define k3  0.166666666667
#define k4  0.0416666666667

#define EXP_TAYLOR_5(x) (1 + x + x * x * k2 + x * x * x * k3 + x * x * x * x * k4)
#define EXP_DIV  100. // 5.
// TODO: Implement interpolation between taylors series as different points

double exp(double x)
{
    long long k = (long long)x;
    double p = x - (double)k;
    return intpow(M_E, k) * intpow(EXP_TAYLOR_5(p / EXP_DIV), EXP_DIV);
}

#define LOG_HALLEY_ITERATIONS   10

double log(double x)
{
    if (fabs(x) < 1e-40)
        return -INFINITY;
    if (x < 0)
        return 0;
    double y = 0;
    for (unsigned char i = 0; i < LOG_HALLEY_ITERATIONS; i++)
        y += 2 * (x - exp(y)) / (x + exp(y));
    return y;
}

double pow(double a, double b)
{
    return exp(b * log(a));
}