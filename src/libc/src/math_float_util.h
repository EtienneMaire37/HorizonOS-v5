#pragma once

typedef union 
{
    float f;
    uint32_t u;
} float_union;

#define DECOMPOSE_FLOAT(value, sign, exponent, mantissa) do { \
    float_union fu = {.f = (value)}; \
    (sign) = (fu.u >> 31) & 1; \
    uint32_t exp_stored = (fu.u >> 23) & 0xff; \
    uint32_t mant_stored = fu.u & 0x7fffff; \
    (exponent) = exp_stored ? (exp_stored - 127) : -126; \
    (mantissa) = exp_stored ? (0x00800000 | mant_stored) : mant_stored; \
} while(0)

#define RECOMPOSE_FLOAT(sign, exponent, mantissa, result) do { \
    uint32_t s = (sign) & 1; \
    int32_t exp = (exponent); \
    uint32_t mant = (mantissa); \
    \
    uint32_t exp_stored, mant_stored; \
    if (mant == 0) \
    { \
        exp_stored = 0; \
        mant_stored = 0; \
    } else if (mant & 0x00800000) \
    { \
        exp_stored = exp + 127; \
        mant_stored = mant & 0x7fffff; \
        if (exp_stored >= 0xff) \
        { \
            exp_stored = 0xff; \
            mant_stored = 0; \
        } \
    } else \
    { \
        exp_stored = 0; \
        mant_stored = mant & 0x7fffff; \
    } \
    float_union fu = {.u = (s << 31) | (exp_stored << 23) | mant_stored}; \
    (result) = fu.f; \
} while(0)

typedef union 
{
    double d;
    uint64_t u;
} double_union;

#define DECOMPOSE_DOUBLE(value, sign, exponent, mantissa) do { \
    double_union du = {.d = (value)}; \
    (sign) = (du.u >> 63) & 1; \
    uint64_t exp_stored = (du.u >> 52) & 0x7ff; \
    uint64_t mant_stored = du.u & 0xfffffffffffff; \
    (exponent) = exp_stored ? (exp_stored - 1023) : -1022; \
    (mantissa) = exp_stored ? (0x10000000000000 | mant_stored) : mant_stored; \
} while(0)

#define RECOMPOSE_DOUBLE(sign, exponent, mantissa, result) do { \
    uint64_t s = (sign) & 1; \
    int64_t exp = (exponent); \
    uint64_t mant = (mantissa); \
    \
    uint64_t exp_stored, mant_stored; \
    if (mant == 0) \
    { \
        exp_stored = 0; \
        mant_stored = 0; \
    } else if (mant & 0x10000000000000) \
    { \
        exp_stored = exp + 1023; \
        mant_stored = mant & 0xfffffffffffff; \
        if (exp_stored >= 0x7ff) \
        { \
            exp_stored = 0x7ff; \
            mant_stored = 0; \
        } \
    } else \
    { \
        exp_stored = 0; \
        mant_stored = mant & 0xfffffffffffff; \
    } \
    double_union du = {.u = (s << 63) | (exp_stored << 52) | mant_stored}; \
    (result) = du.d; \
} while(0)

// ^ 80bit FPU long double
typedef struct 
{
    uint64_t mantissa;
    uint16_t sign_exp;
} __attribute__((packed)) long_double_parts;

typedef union 
{
    long double ld;
    long_double_parts parts;
} long_double_union;

#define DECOMPOSE_LONG_DOUBLE(value, sign, exponent, _mantissa) do { \
    long_double_union ldu = {.ld = (value)}; \
    (sign) = (ldu.parts.sign_exp >> 15) & 1; \
    uint16_t exp_stored = ldu.parts.sign_exp & 0x7fff; \
    (exponent) = exp_stored ? (exp_stored - 16383) : -16382; \
    (_mantissa) = ldu.parts.mantissa; \
} while(0)

#define RECOMPOSE_LONG_DOUBLE(sign, exponent, _mantissa, result) do { \
    uint16_t s = (sign) & 1; \
    int32_t exp = (exponent); \
    uint64_t mant = (_mantissa); \
    \
    uint16_t exp_stored; \
    if (exp == -16382) \
    { \
        exp_stored = 0; \
        mant &= 0x7fffffffffffffff; \
    } else \
    { \
        exp_stored = exp + 16383; \
        if (exp_stored >= 0x7fff) \
        { \
            exp_stored = 0x7fff; \
            mant = 0x8000000000000000; \
        } \
    } \
    long_double_union ldu; \
    ldu.parts.mantissa = mant; \
    ldu.parts.sign_exp = (s << 15) | exp_stored; \
    (result) = ldu.ld; \
} while(0)
