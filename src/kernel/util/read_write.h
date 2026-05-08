#pragma once

#include <stdint.h>
#include "../cpu/memory.h"
#include "scalar.h"

#define READ_ONCE(x)                                                            \
({                                                                              \
    if (!is_scalar(x))                                                          \
        memory_barrier();                                                       \
    typeof(x) ___x;                                                             \
    volatile __attribute__((__may_alias__)) typeof(x)* ptr =                    \
    (volatile __attribute__((__may_alias__)) typeof(x)*)&(x);                   \
    if (!is_scalar(x))                                                          \
        __builtin_memcpy((void*)&___x, (const void*)ptr, sizeof(x));            \
    else                                                                        \
        ___x = *ptr;                                                            \
    if (!is_scalar(x))                                                          \
        memory_barrier();                                                       \
    ___x;                                                                       \
})
#define WRITE_ONCE(x, val)                                                      \
({                                                                              \
    if (!is_scalar(x))                                                          \
        memory_barrier();                                                       \
    volatile __attribute__((__may_alias__)) typeof(x)* ptr =                    \
    (volatile __attribute__((__may_alias__)) typeof(x)*)&(x);                   \
    typeof(val) ___tmp = (val);                                                 \
    if (!is_scalar(x))                                                          \
        __builtin_memcpy((void*)ptr, (const void*)&___tmp, sizeof(x));          \
    else                                                                        \
        *ptr = val;                                                             \
    if (!is_scalar(x))                                                          \
        memory_barrier();                                                       \
})
