#pragma once

#ifndef __CURRENT_FUNC__

#if __STDC_VERSION__ >= 199901L
#define __CURRENT_FUNC__    __func__
#elif __GNUC__ >= 2
#define __CURRENT_FUNC__    __FUNCTION__
#else
#define __CURRENT_FUNC__    ""
#endif

#endif

#ifdef NDEBUG
#define assert() ((void) 0)
#else
#ifdef BUILDING_KERNEL
void __attribute__((noreturn)) abort_core(int line, const char* file, const char* function);
#define abort() abort_core(__LINE__, __CURRENT_FUNC__, __FILE__)
#else
void __attribute__((noreturn)) abort();
#endif
int dprintf(int fd, const char*format, ...);
#define assert(val) ((val) ? (void)0 : (dprintf(STDERR_FILENO, "%s:%u: %s: Assertion `%s` failed.\n", __FILE__, (int)__LINE__, __CURRENT_FUNC__, #val), abort()))
#endif