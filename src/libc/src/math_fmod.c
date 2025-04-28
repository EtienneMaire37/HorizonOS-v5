#pragma once

double frexp(double x, int *exp)
{
    if (x == 0.)
    {
        *exp = 0;
        return 0.;
    }

    uint64_t sign;
    int64_t exponent;
    uint64_t mantissa;
    DECOMPOSE_DOUBLE(x, sign, exponent, mantissa);

    const int64_t denormal_exp = -1022;
    if (exponent != denormal_exp)
    {
        *exp = exponent + 1;
        double m = (double)mantissa / (1ULL << 53);
        return sign ? -m : m;
    }
    else
    {
        if (mantissa == 0)
        {
            *exp = 0;
            return 0.;
        }

        uint64_t m = mantissa;
        int lz = 0;
        while ((m & 0x0008000000000000) == 0 && lz < 52)
        {
            lz++;
            m <<= 1;
        }

        *exp = denormal_exp - lz;
        double factor = (double)(1ULL << (lz + 1));
        double m_val = (double)mantissa * factor / (1ULL << 53);
        return sign ? -m_val : m_val;
    }
}

float frexpf(float x, int *exp)
{
    if (x == 0.f)
    {
        *exp = 0;
        return 0.f;
    }

    uint32_t sign;
    int32_t exponent;
    uint32_t mantissa;
    DECOMPOSE_FLOAT(x, sign, exponent, mantissa);

    const int32_t denormal_exp = -126;
    if (exponent != denormal_exp)
    {
        *exp = exponent + 1;
        float m = (float)mantissa / (1UL << 24);
        return sign ? -m : m;
    }
    else
    {
        if (mantissa == 0)
        {
            *exp = 0;
            return 0.f;
        }

        uint32_t m = mantissa;
        int lz = 0;
        while ((m & 0x00400000) == 0 && lz < 23)
        {
            lz++;
            m <<= 1;
        }

        *exp = denormal_exp - lz;
        float factor = (float)(1U << (lz + 1));
        float m_val = (float)mantissa * factor / (1UL << 24);
        return sign ? -m_val : m_val;
    }
}

long double frexpl(long double x, int *exp)
{
    if (x == 0.L)
    {
        *exp = 0;
        return 0.L;
    }

    uint16_t sign;
    int32_t exponent;
    uint64_t mantissa;
    DECOMPOSE_LONG_DOUBLE(x, sign, exponent, mantissa);

    const int32_t denormal_exp = -16382;
    if (exponent != denormal_exp)
    {
        *exp = exponent + 1;
        long double m = (long double)mantissa / 0x1p64L;
        return sign ? -m : m;
    }
    else
    {
        if (mantissa == 0)
        {
            *exp = 0;
            return 0.L;
        }

        uint64_t m = mantissa;
        int lz = 0;
        while ((m & 0x8000000000000000) == 0 && lz < 63)
        {
            lz++;
            m <<= 1;
        }

        *exp = denormal_exp - lz;
        long double factor = (long double)(1ULL << (lz + 1));
        long double m_val = (long double)mantissa * factor / 0x1p64L;
        return sign ? -m_val : m_val;
    }
}

double copysign(double x, double y)
{
    uint64_t sign_x, sign_y;
    int64_t exp_x;
    uint64_t mant_x;
    int64_t dummy_exp;
    uint64_t dummy_mant;
    
    DECOMPOSE_DOUBLE(x, sign_x, exp_x, mant_x);
    (void)sign_x;
    
    DECOMPOSE_DOUBLE(y, sign_y, dummy_exp, dummy_mant);
    (void)dummy_exp;
    (void)dummy_mant;

    double result;
    RECOMPOSE_DOUBLE(sign_y, exp_x, mant_x, result);
    return result;
}

float copysignf(float x, float y)
{
    uint32_t sign_x, sign_y;
    int32_t exp_x;
    uint32_t mant_x;
    int32_t dummy_exp;
    uint32_t dummy_mant;
    
    DECOMPOSE_FLOAT(x, sign_x, exp_x, mant_x);
    (void)sign_x;
    
    DECOMPOSE_FLOAT(y, sign_y, dummy_exp, dummy_mant);
    (void)dummy_exp;
    (void)dummy_mant;

    float result;
    RECOMPOSE_FLOAT(sign_y, exp_x, mant_x, result);
    return result;
}

long double copysignl(long double x, long double y)
{
    uint16_t sign_x, sign_y;
    int32_t exp_x;
    uint64_t mant_x;
    int32_t dummy_exp;
    uint64_t dummy_mant;
    
    DECOMPOSE_LONG_DOUBLE(x, sign_x, exp_x, mant_x);
    (void)sign_x;
    
    DECOMPOSE_LONG_DOUBLE(y, sign_y, dummy_exp, dummy_mant);
    (void)dummy_exp;
    (void)dummy_mant;

    long double result;
    RECOMPOSE_LONG_DOUBLE(sign_y, exp_x, mant_x, result);
    return result;
}

double ldexp(double x, int exp)
{
    if (x == 0.) return 0.;
    
    uint64_t sign;
    int64_t old_exp;
    uint64_t mantissa;
    DECOMPOSE_DOUBLE(x, sign, old_exp, mantissa);
    
    int64_t new_exp = old_exp + exp;
    double result;
    RECOMPOSE_DOUBLE(sign, new_exp, mantissa, result);
    return result;
}

float ldexpf(float x, int exp)
{
    if (x == 0.f) return 0.f;
    
    uint32_t sign;
    int32_t old_exp;
    uint32_t mantissa;
    DECOMPOSE_FLOAT(x, sign, old_exp, mantissa);
    
    int32_t new_exp = old_exp + exp;
    float result;
    RECOMPOSE_FLOAT(sign, new_exp, mantissa, result);
    return result;
}

long double ldexpl(long double x, int exp)
{
    if (x == 0.L) return 0.L;
    
    uint16_t sign;
    int32_t old_exp;
    uint64_t mantissa;
    DECOMPOSE_LONG_DOUBLE(x, sign, old_exp, mantissa);
    
    int32_t new_exp = old_exp + exp;
    long double result;
    RECOMPOSE_LONG_DOUBLE(sign, new_exp, mantissa, result);
    return result;
}

double fmod(double x, double y)
{
    if (y == 0.L || isinf(x))
    {
        errno = EDOM;
        return NAN;
    }
    if (isinf(y))
        return x;

    int exp_x, exp_y;
    double m_x = frexp(x, &exp_x);
    double m_y = frexp(y, &exp_y);

    while (exp_x > exp_y)
    {
        m_y /= 2;
        exp_y++;
    }
    while (exp_y > exp_x)
    {
        m_x /= 2;
        exp_x++;
    }

    int64_t quotient = (int64_t)(m_x / m_y);
    double remainder = m_x - m_y * quotient;
    
    return remainder == 0.L ? copysign(0.L, x) : ldexp(remainder, exp_x);
}

long double fmodl(long double x, long double y)
{
    if (y == 0.L || isinf(x)) 
    {
        errno = EDOM;
        return NAN;
    }
    if (isinf(y))
        return x;

    int exp_x, exp_y;
    long double m_x = frexpl(x, &exp_x);
    long double m_y = frexpl(y, &exp_y);

    while (exp_x > exp_y) 
    {
        m_y /= 2.L;
        exp_y++;
    }
    while (exp_y > exp_x) 
    {
        m_x /= 2.L;
        exp_x++;
    }

    int64_t quotient = (int64_t)(m_x / m_y);
    long double remainder = m_x - m_y * quotient;
    
    return remainder == 0.L ? copysignl(0.L, x) : ldexpl(remainder, exp_x);
}

double floor(double x)
{
    if (isnan(x) || isinf(x))
        return x;
    
    uint64_t sign, mantissa;
    int64_t exponent;
    DECOMPOSE_DOUBLE(x, sign, exponent, mantissa);

    if (exponent < 0) 
    {
        if (x == 0.0) return x;
        return sign ? -1.0 : 0.0;
    }

    uint64_t stored_mantissa = mantissa & 0xfffffffffffffULL;
    int bits_to_mask = 52 - exponent;

    if (bits_to_mask <= 0) return x;

    uint64_t mask = ~((1ULL << bits_to_mask) - 1);
    uint64_t truncated_stored = stored_mantissa & mask;
    uint64_t truncated_mantissa = (mantissa & 0xfff0000000000000ULL) | truncated_stored;

    double result;
    RECOMPOSE_DOUBLE(sign, exponent, truncated_mantissa, result);

    if (truncated_stored != stored_mantissa && sign)
        result -= 1.0;

    return result;
}

float floorf(float x)
{
    if (isnan(x) || isinf(x))
        return x;
    
    uint32_t sign, mantissa;
    int32_t exponent;
    DECOMPOSE_FLOAT(x, sign, exponent, mantissa);

    if (exponent < 0) 
    {
        if (x == 0.f) return x;
        return sign ? -1.f : 0.f;
    }

    uint32_t stored_mantissa = mantissa & 0x7fffffU;
    int bits_to_mask = 23 - exponent;

    if (bits_to_mask <= 0) return x;

    uint32_t mask = ~((1U << bits_to_mask) - 1);
    uint32_t truncated_stored = stored_mantissa & mask;
    uint32_t truncated_mantissa = (mantissa & 0xff800000U) | truncated_stored;

    float result;
    RECOMPOSE_FLOAT(sign, exponent, truncated_mantissa, result);

    if (truncated_stored != stored_mantissa && sign)
        result -= 1.f;

    return result;
}

long double floorl(long double x)
{
    if (isnan(x) || isinf(x))
        return x;
    
    uint16_t sign;
    int32_t exponent;
    uint64_t mantissa;
    DECOMPOSE_LONG_DOUBLE(x, sign, exponent, mantissa);

    if (exponent < 0) 
    {
        if (x == 0.L) return x;
        return sign ? -1.L : 0.L;
    }

    int bits_to_mask = 63 - exponent;
    if (bits_to_mask <= 0) return x;

    uint64_t mask = ~((1ULL << bits_to_mask) - 1);
    uint64_t truncated_mantissa = mantissa & mask;

    long double result;
    RECOMPOSE_LONG_DOUBLE(sign, exponent, truncated_mantissa, result);

    if (truncated_mantissa != mantissa && sign)
        result -= 1.L;

    return result;
}