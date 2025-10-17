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
#define assert(ignore) ((void) 0)
#else
void abort(void);
int dprintf(int fd, const char*format, ...);
#define assert(val) ((val) ? (void)0 : (dprintf(STDERR_FILENO, "%s:%u: %s: Assertion `%s` failed.\n", __FILE__, __LINE__, __CURRENT_FUNC__, #val), abort()))
#endif