#include "../debug/out.h"
#include <stdint.h>
#include <stdlib.h>
#include "../cpu/util.h"

#define printlog(...) do { LOG(ERROR, __VA_ARGS__); printf(__VA_ARGS__); putchar('\n'); } while (0)

// * From https://wiki.osdev.org/Undefined_Behavior_Sanitization
struct source_location
{
    const char* file;
    uint32_t line;
    uint32_t column;
};
struct type_descriptor
{
    uint16_t kind;
    uint16_t info;
    char name[];
};
struct type_mismatch_v1_info
{
    struct source_location location;
    struct type_descriptor* type;
    uint8_t alignment;
    uint8_t type_check_kind;
};

static inline void log_location_and_abort(struct source_location* loc)
{
    printlog("ubsan source location: file \"%s\", line %d, column %d", loc->file, loc->line, loc->column);
    abort();
}

const char* type_check_kinds[] =
{
    "Load of",
    "Store to",
    "Reference binding to",
    "Member access within",
    "Member call on",
    "Constructor call on",
    "Downcast of",
    "Downcast of",
    "Upcast of",
    "Cast to virtual base of",
};

#define log_ubsan_error() do { disable_interrupts(); printf("\x1b[31mUBSAN:\x1b[0m\n"); } while (0)
#define DEFINE_UBSAN_SYMBOL_FALLBACK(sym) void __attribute__((used)) sym(struct source_location* loc) { log_ubsan_error(); printlog("ubsan runtime error: " #sym); log_location_and_abort(loc); }

DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_add_overflow)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_sub_overflow)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_negate_overflow)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_mul_overflow)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_divrem_overflow)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_load_invalid_value)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_shift_out_of_bounds)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_vla_bound_not_positive)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_pointer_overflow)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_out_of_bounds)
DEFINE_UBSAN_SYMBOL_FALLBACK(__ubsan_handle_invalid_builtin)
void __attribute__((used)) __ubsan_handle_type_mismatch_v1(struct type_mismatch_v1_info* info, void* ptr)
{
    log_ubsan_error();
    if (ptr == NULL)
    {
        printlog("NULL pointer dereference");
    }
                                                // * logarithmic alignment
    else if (info->alignment != 0 && ((uintptr_t)ptr & ((1ULL << info->alignment) - 1)))
    {
        printlog("Address %p does not have sufficient alignment %llu", ptr, 1ULL << info->alignment);
    }
    else
    {
        printlog("%s address %p with insufficient space for object of type %s",
            type_check_kinds[info->type_check_kind], ptr,
            info->type->name);
    }
    log_location_and_abort(&info->location);
}
