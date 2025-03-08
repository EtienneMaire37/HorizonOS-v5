#include "../include/math.h"
#include "../include/inttypes.h"

#define BIN_SEARCH_PRECISION    .00000000001

double fabs(double x)
{
    return x < 0 ? -x : x;
}
float fabsf(float x)
{
    return x < 0 ? -x : x;
}
long double fabsl(long double x)
{
    return x < 0 ? -x : x;
}

double fmod(double a, double b)
{
    while (a >= b || a < 0)
        a += b * (a < 0 ? 1 : -1);
    return a;
}
float fmodf(float a, float b)
{
    while (a >= b || a < 0)
        a += b * (a < 0 ? 1 : -1);
    return a;
}
long double fmodl(long double a, long double b)
{
    while (a >= b || a < 0)
        a += b * (a < 0 ? 1 : -1);
    return a;
}

double intpow(double a, int64_t b)
{
    if (b == 0)
        return 1;
    if (b < 0)
        return intpow(a, -b);

    double result = 1.;
    for (int64_t i = 0; i < b; i++)
        result *= a;
    return result;
}
float intpowf(float a, int64_t b)
{
    if (b == 0)
        return 1;
    if (b < 0)
        return intpowf(a, -b);

        float result = 1.;
    for (int64_t i = 0; i < b; i++)
        result *= a;
    return result;
}
long double intpowl(long double a, int64_t b)
{
    if (b == 0)
        return 1;
    if (b < 0)
        return intpowl(a, -b);

    long double result = 1.;
    for (int64_t i = 0; i < b; i++)
        result *= a;
    return result;
}

#define exp_k2  0.5
#define exp_k3  0.166666666667
#define exp_k4  0.0416666666667
#define exp_k5  0.00833333333333

#define EXP_TAYLOR_5(x) (1 + (x) * (1 + (x) * (exp_k2 + (x) * (exp_k3 + (x) * (exp_k4 + (x) * exp_k5))))) // (1 + x + x * x * k2 + x * x * x * k3 + x * x * x * x * k4)
#define EXP_DIV  100. // 5.
#define M_SQRTE  1.6487212707001281941643356
// TODO: Implement interpolation between taylors series as different points

double exp(double x)
{
    int64_t k = (int64_t)x;
    double p = x - (double)k;
    return intpow(M_E, k) * intpow(EXP_TAYLOR_5(p / EXP_DIV), EXP_DIV);
}
float expf(float x)
{
    int64_t k = (int64_t)x;
    float p = x - (float)k;
    return intpowf((float)M_E, k) * intpowf(EXP_TAYLOR_5(p / EXP_DIV), EXP_DIV);
}
long double expl(long double x)
{
    int64_t k = (int64_t)x;
    long double p = x - (long double)k;
    return intpowl(M_El, k) * intpowl(EXP_TAYLOR_5(p / EXP_DIV), EXP_DIV);
}

double exp2(double x)
{
    int64_t k = (int64_t)x;
    double p = x - (double)k;
    if (fabs(p) < 1e-50) // if (p < 1e-50)
        return intpow(2, k);
    return pow(2, x);   // Not the best way to do it
}
float exp2f(float x)
{
    int64_t k = (int64_t)x;
    float p = x - (float)k;
    if (fabsf(p) < 1e-50)
        return intpowf(2, k);
    return pow(2, x);
}
long double exp2l(long double x)
{
    int64_t k = (int64_t)x;
    long double p = x - (long double)k;
    if (fabsl(p) < 1e-50)
        return intpowl(2, k);
    return powl(2, x);
}

#define LOG_HALLEY_ITERATIONS   17

#define LOG_ITERATIONS(x)   ((x) < .01 ? 18 : ((x) < .1 ? 25 : ((x) < 1 ? 7 : ((x) < 10 ? 10 : ((x) < 100 ? 12 : ((x) < 1000 ? 14 : ((x) < 100000 ? 15 : 18)))))))

double log(double x)
{
    if (fabs(x) < 1e-40)
        return -INFINITY;
    if (x < 0)
        return 0;
    double y = 1;
    uint8_t iterations = LOG_ITERATIONS(x);
    // for (uint8_t i = 0; i < LOG_HALLEY_ITERATIONS; i++)
    for (uint8_t i = 0; i < iterations; i++)
        y += 2 * (x - exp(y)) / (x + exp(y));
    return y;
    // double xmin = 0, xmax = x;
    // while (xmin < xmax)
    // {
    //     double mid = (xmin + xmax) * .5;
    //     double e_mid = exp(mid);
    //     if (e_mid < x)
    //     {
    //         xmin = mid + BIN_SEARCH_PRECISION;
    //     }
    //     else if (e_mid > x)
    //     {
    //         xmax = mid - BIN_SEARCH_PRECISION;
    //     }
    // }
    // return xmin;
}
float logf(float x)
{
    if (fabsf(x) < 1e-40)
        return -INFINITY;
    if (x < 0)
        return 0;
    float y = 1;
    uint8_t iterations = LOG_ITERATIONS(x);
    for (uint8_t i = 0; i < iterations; i++)
        y += 2 * (x - expf(y)) / (x + expf(y));
    return y;
}
long double logl(long double x)
{
    if (fabsl(x) < 1e-40)
        return -INFINITY;
    if (x < 0)
        return 0;
    long double y = 1;
    uint8_t iterations = LOG_ITERATIONS(x);
    for (uint8_t i = 0; i < iterations; i++)
        y += 2 * (x - expl(y)) / (x + expl(y));
    return y;
}

double pow(double a, double b)
{
    return exp(b * log(a));
}
float powf(float a, float b)
{
    return expf(b * logf(a));
}
long double powl(long double a, long double b)
{
    return expl(b * logl(a));
}

#define SIN_TAYLOR_4(xx) ((xx) - ((xx) * (xx) * (xx)) / 6. + ((xx) * (xx) * (xx) * (xx) * (xx)) / 120.)

inline double _sin(double x) // -M_PI/4 <= x <= M_PI/4
{
    double ax = fabs(x);

    if (ax < 1.49011612e-8) // ~2^-26
        return x;

    if (ax < 0.26179938779) // M_PI/12
    {
        double x3 = x * x * x;
        double x5 = x3 * x * x;
        return x - x3 / 6. + x5 / 120.;
    } 
    else 
    {
        double x2 = x * x;
        double x3 = x2 * x;
        double x5 = x3 * x2;
        double x7 = x5 * x2;
        return x - x3 / 6. + x5 / 120. - x7 / 5040.;
    }
}

inline double _cos(double x) // -M_PI/4 <= x <= M_PI/4
{
    double ax = fabs(x);

    if (ax < 1.49011612e-8)
        return 1.;

    double x2 = x * x;
    double x4 = x2 * x2;
    double x6 = x4 * x2;

    if (ax < 0.26179938779) // M_PI/12
        return 1. - x2 / 2. + x4 / 24. - x6 / 720.;
    else 
    {
        double x8 = x6 * x2;
        return 1. - x2 / 2. + x4 / 24. - x6 / 720. + x8 / 40320.;
    }
}

double sin(double x) 
{
    if (isnan(x)) return x;
    if (isinf(x)) return NAN;

    double two_pi = 2. * M_PI;
    double sign = 1.;

    x = fmod(x, two_pi);
    if (x < 0) 
    {
        x = -x;
        sign = -1.;
    }

    int quadrant = (int)(x / (M_PI / 2));
    double y = x - quadrant * (M_PI / 2);
    quadrant %= 4;

    double sin_y, cos_y;
    if (y <= M_PI / 4) {
        sin_y = _sin(y);
        cos_y = _cos(y);
    } 
    else 
    {
        double z = M_PI / 2 - y;
        sin_y = _cos(z);
        cos_y = _sin(z);
    }

    switch (quadrant) 
    {
        case 0: return sign * sin_y;
        case 1: return sign * cos_y;
        case 2: return sign * -sin_y;
        case 3: return sign * -cos_y;
        default: return 0.;
    }
}

double cos(double x) 
{
    if (isnan(x)) return x;
    if (isinf(x)) return NAN;

    double two_pi = 2. * M_PI;

    x = fmod(x, two_pi);
    x = fabs(x);

    int quadrant = (int)(x / (M_PI / 2));
    double y = x - quadrant * (M_PI / 2);
    quadrant %= 4;

    double sin_y, cos_y;
    if (y <= M_PI / 4) 
    {
        sin_y = _sin(y);
        cos_y = _cos(y);
    } 
    else 
    {
        double z = M_PI / 2 - y;
        sin_y = _cos(z);
        cos_y = _sin(z);
    }

    switch (quadrant) 
    {
        case 0: return cos_y;
        case 1: return -sin_y;
        case 2: return -cos_y;
        case 3: return sin_y;
        default: return 0.;
    }
}

double tan(double x)
{
    return sin(x) / cos(x);
}
// float tanf(float x)
// {
//     return sinf(x) / cosf(x);
// }
// long double tanl(long double x)
// {
//     return sinl(x) / cosl(x);
// }

double sqrt(double x)
{
    double xmin = 0, xmax = x;
    while (xmin < xmax)
    {
        double mid = (xmin + xmax) * .5;
        double mid2 = mid * mid;
        if (mid2 < x)
        {
            xmin = mid + BIN_SEARCH_PRECISION;
        }
        else if (mid2 > x)
        {
            xmax = mid - BIN_SEARCH_PRECISION;
        }
    }
    return xmin;
}

// float sqrtf(float x);
// long double sqrtl(long double x);