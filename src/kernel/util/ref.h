#pragma once

// From https://nullprogram.com/blog/2015/02/17/

#include <stddef.h>
#include "assert.h"

struct ref {
    void (*free)(const struct ref *);
    int count;
};

static inline void
ref_inc(const struct ref *ref)
{
    ASSERT(ref);
    __sync_add_and_fetch((int *)&ref->count, 1);
}

static inline void
ref_dec(const struct ref *ref)
{
    ASSERT(ref);
    if (__sync_sub_and_fetch((int *)&ref->count, 1) == 0)
        ref->free(ref);
}

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
