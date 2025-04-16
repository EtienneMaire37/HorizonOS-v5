#include "../include/math.h"
#include "../include/errno_defs.h"
extern int errno;
#include <stdint.h>

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

double frexp(double x, int *exp)
{
    if (x == 0.)
    {
        *exp = 0;
        return 0.;
    }
    uint64_t bits = *(uint64_t*)&x;
    int sign = (bits >> 63) & 1;
    int exponent = (bits >> 52) & 0x7FF;
    uint64_t mantissa = bits & 0x000FFFFFFFFFFFFFULL;

    if (exponent == 0x7FF)
    {
        *exp = 0;
        return x;
    }

    if (exponent == 0)
    {
        exponent = -1022;
        while ((mantissa & 0x0008000000000000ULL) == 0)
        {
            mantissa <<= 1;
            exponent--;
        }
        mantissa <<= 1;
        mantissa &= 0x000FFFFFFFFFFFFFULL;
        exponent--;
    }
    else
    {
        exponent -= 1023;
    }

    *exp = exponent + 1;
    uint64_t new_bits = ((uint64_t)sign << 63) | ((uint64_t)(1022) << 52) | mantissa;
    return *(double*)&new_bits;
}

float frexpf(float x, int *exp)
{
    if (x == 0.0f)
    {
        *exp = 0;
        return 0.0f;
    }
    uint32_t bits = *(uint32_t*)&x;
    int sign = (bits >> 31) & 1;
    int exponent = (bits >> 23) & 0xFF;
    uint32_t mantissa = bits & 0x007FFFFFUL;

    if (exponent == 0xFF)
    {
        *exp = 0;
        return x;
    }

    if (exponent == 0)
    {
        exponent = -126;
        while ((mantissa & 0x00400000UL) == 0)
        {
            mantissa <<= 1;
            exponent--;
        }
        mantissa <<= 1;
        mantissa &= 0x007FFFFFUL;
        exponent--;
    }
    else
    {
        exponent -= 127;
    }

    *exp = exponent + 1;
    uint32_t new_bits = ((uint32_t)sign << 31) | ((uint32_t)(126) << 23) | mantissa;
    return *(float*)&new_bits;
}

long double frexpl(long double x, int *exp)
{
    if (x == 0.L)
    {
        *exp = 0;
        return 0.L;
    }
    uint16_t *parts = (uint16_t*)&x;
    int sign = (parts[4] >> 15) & 1;
    int exponent = parts[4] & 0x7FFF;
    uint64_t mantissa = *(uint64_t*)parts;

    if (exponent == 0x7FFF)
    {
        *exp = 0;
        return x;
    }

    if (exponent == 0)
    {
        exponent = -16382;
        while ((mantissa & 0x8000000000000000ULL) == 0)
        {
            mantissa <<= 1;
            exponent--;
        }
        exponent--;
    }
    else
    {
        exponent -= 16383;
    }

    *exp = exponent + 1;
    parts[4] = (sign << 15) | (16382);
    *(uint64_t*)parts = mantissa;
    return x;
}

double intpow(double a, int64_t b)
{
    if (b == 0)
        return 1;
    if (b < 0)
        return 1 / intpow(a, -b);

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

    float result = 1.f;
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

#define EXP_TAYLOR_5(x) (1 + (x) * (1 + (x) * (exp_k2 + (x) * (exp_k3 + (x) * (exp_k4 + (x) * exp_k5)))))
#define EXP_DIV  100.
#define M_SQRTE  1.6487212707001281941643356

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
    if (fabs(p) < 1e-50)
        return intpow(2, k);
    return pow(2, x);
}
float exp2f(float x)
{
    int64_t k = (int64_t)x;
    float p = x - (float)k;
    if (fabsf(p) < 1e-50)
        return intpowf(2, k);
    return powf(2, x);
}
long double exp2l(long double x)
{
    int64_t k = (int64_t)x;
    long double p = x - (long double)k;
    if (fabsl(p) < 1e-50)
        return intpowl(2, k);
    return powl(2, x);
}

double log(double x)
{
    if (x < 0 || x == -INFINITY)
    {
        errno = EDOM;
        return NAN;
    }
    if (x == 0.)
    {
        errno = ERANGE;
        return -HUGE_VAL;
    }
    if (x == INFINITY)
    {
        return x;
    }

    int _exp;
    double m = frexp(x, &_exp);
    if (m < 0.5)
    {
        m *= 2;
        _exp--;
    }

    double y = 0.;
    for (int i = 0; i < 4; i++)
    {
        double ey = exp(y);
        y += 2 * (m - ey) / (m + ey);
    }
    return y + _exp * M_LN2;
}

float logf(float x)
{
    if (x < 0 || x == -INFINITY)
    {
        errno = EDOM;
        return NAN;
    }
    if (x == 0.0f)
    {
        errno = ERANGE;
        return -HUGE_VALF;
    }
    if (x == INFINITY)
    {
        return x;
    }

    int _exp;
    float m = frexpf(x, &_exp);
    if (m < 0.5f)
    {
        m *= 2;
        _exp--;
    }

    float y = 0.0f;
    for (int i = 0; i < 4; i++)
    {
        float ey = expf(y);
        y += 2 * (m - ey) / (m + ey);
    }
    return y + _exp * (float)M_LN2;
}

long double logl(long double x)
{
    if (x < 0 || x == -INFINITY)
    {
        errno = EDOM;
        return NAN;
    }
    if (x == 0.L)
    {
        errno = ERANGE;
        return -HUGE_VALL;
    }
    if (x == INFINITY)
    {
        return x;
    }

    int _exp;
    long double m = frexpl(x, &_exp);
    if (m < 0.5L)
    {
        m *= 2;
        _exp--;
    }

    long double y = 0.L;
    for (int i = 0; i < 4; i++)
    {
        long double ey = expl(y);
        y += 2 * (m - ey) / (m + ey);
    }
    return y + _exp * M_LN2l;
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

inline double _sin(double x)
{
    double ax = fabs(x);
    if (ax < 1.49011612e-8)
        return x;
    if (ax < 0.26179938779)
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

inline double _cos(double x)
{
    double ax = fabs(x);
    if (ax < 1.49011612e-8)
        return 1.;
    double x2 = x * x;
    double x4 = x2 * x2;
    double x6 = x4 * x2;
    if (ax < 0.26179938779)
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

double sqrt(double x)
{
    if (x < -0.)
    {
        errno = EDOM;
        return NAN;
    }
    if (x == 0) return 0;
    if (x == 1) return 1;
    double xmin, xmax;
    if (x > 1) { xmin = 0; xmax = x; }
    else       { xmin = x; xmax = 1; }
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

double sinh(double x)
{
    return .5 * (exp(x) - exp(-x));
}
double cosh(double x)
{
    return .5 * (exp(x) + exp(-x));
}
double tanh(double x)
{
    return (exp(x) - exp(-x)) / (exp(x) + exp(-x));
}