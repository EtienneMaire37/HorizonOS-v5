#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include "../cpu/registers.h"
#include "../cpu/util.h"
#include "../util/error.h"

typedef atomic_flag spinlock_t;
typedef atomic_flag spinlock_noint_t;

#define SPINLOCK_INIT           ATOMIC_FLAG_INIT
#define SPINLOCK_NOINT_INIT     ATOMIC_FLAG_INIT

static inline __attribute__((always_inline)) bool try_acquire_spinlock(spinlock_t* spinlock)
{
	return atomic_flag_test_and_set_explicit(spinlock, memory_order_acquire);
}

static inline __attribute__((always_inline)) void acquire_spinlock(spinlock_t* spinlock)
{
	while (try_acquire_spinlock(spinlock))
    {
        FATAL("DEADLOCK");
        __builtin_ia32_pause();
    }
}

static inline __attribute__((always_inline)) void release_spinlock(spinlock_t* spinlock)
{
	atomic_flag_clear_explicit(spinlock, memory_order_release);
}

uint32_t acquire_spinlock_noint(spinlock_noint_t* spinlock);
bool try_acquire_spinlock_noint(spinlock_noint_t* spinlock, uint32_t* eflags);
void release_spinlock_noint(spinlock_noint_t* spinlock, uint32_t eflags);
