#pragma once

#define evaluate(...) _evaluate(1, __VA_ARGS__)

static inline void _evaluate(int a, ...)
{
    (void)a;
}
