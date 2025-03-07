#include "../include/math.h"
#include "../include/inttypes.h"

double fabs(double x)
{
    return x < 0 ? -x : x;
}

double fmod(double a, double b)
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

#define exp_k2  0.5
#define exp_k3  0.166666666667
#define exp_k4  0.0416666666667

#define EXP_TAYLOR_5(x) (1 + (x) * (1 + (x) * exp_k2 + (x) * ((x) * exp_k3 + (x) * (x) * exp_k4))) // (1 + x + x * x * k2 + x * x * x * k3 + x * x * x * x * k4)
#define EXP_DIV  100. // 5.
#define M_SQRTE  1.6487212707001281941643356
// TODO: Implement interpolation between taylors series as different points

double exp(double x)
{
    int64_t k = (int64_t)x;
    double p = x - (double)k;
    return intpow(M_E, k) * intpow(EXP_TAYLOR_5(p / EXP_DIV), EXP_DIV);
}

#define LOG_HALLEY_ITERATIONS   17

double log(double x)
{
    if (fabs(x) < 1e-40)
        return -INFINITY;
    if (x < 0)
        return 0;
    double y = 1;
    uint8_t iterations = x < .01 ? 18 : (x < .1 ? 25 : (x < 1 ? 7 : (x < 10 ? 10 : (x < 100 ? 12 : (x < 1000 ? 14 : (x < 100000 ? 15 : 18))))));
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
    //         xmin = mid + .001;
    //     }
    //     else if (e_mid > x)
    //     {
    //         xmax = mid - .001;
    //     }
    // }
    // return xmin;
}

double pow(double a, double b)
{
    return exp(b * log(a));
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