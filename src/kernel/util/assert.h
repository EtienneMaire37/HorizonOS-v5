#pragma once

#ifndef NDEBUG
#include <assert.h>
#define ASSERT(x) assert(x)
#else
// Assume assertions if in release build
// Also evaluate expression
#define ASSERT(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#endif
