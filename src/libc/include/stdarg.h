#pragma once

typedef char* va_list;

// #define va_start(ap, parmn) (void)((ap) = (char*)(&(parmn) + 1))
// #define va_end(ap)          (void)((ap) = 0)
// #define va_arg(ap, type)    (((type*)((ap) = ((ap) + sizeof(type))))[-1])

// ISO C99

// typedef __va_list va_list;

#define va_start(ap, arg)       (__builtin_va_start((ap), (arg)))
#define va_arg		            __builtin_va_arg
#define va_copy		            __builtin_va_copy
#define va_end(ap)              (__builtin_va_end((ap)))