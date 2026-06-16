#include "../util/error.h"

#define DEFINE_UBSAN_SYMBOL(sym) void __attribute__((used)) sym() { FATAL("GCC #UB sanitizer runtime error: " #sym); }

#ifndef LINK_TIME_UB_ERRORS
DEFINE_UBSAN_SYMBOL(__ubsan_handle_add_overflow)
DEFINE_UBSAN_SYMBOL(__ubsan_handle_sub_overflow)
DEFINE_UBSAN_SYMBOL(__ubsan_handle_negate_overflow)
DEFINE_UBSAN_SYMBOL(__ubsan_handle_mul_overflow)
DEFINE_UBSAN_SYMBOL(__ubsan_handle_divrem_overflow)
DEFINE_UBSAN_SYMBOL(__ubsan_handle_load_invalid_value)
DEFINE_UBSAN_SYMBOL(__ubsan_handle_shift_out_of_bounds)
DEFINE_UBSAN_SYMBOL(__ubsan_handle_vla_bound_not_positive)
#endif
