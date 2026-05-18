#pragma once

#include "../cpu/util.h"
#include "../multicore/spinlock.h"
#include <stdint.h>

typedef uint64_t precise_time_t;
#define PRECISE_TIME_MAX        UINT64_MAX

extern atomic_flag time_lock;

extern struct timespec current_time;
extern int64_t tsc_remainder;

extern bool tpause_supported;

extern bool time_initialized;

#define GLOBAL_TIMER_FREQUENCY  1000ULL
#define GLOBAL_TIMER_INCREMENT  (precise_time_ticks_per_second / GLOBAL_TIMER_FREQUENCY)

static const precise_time_t precise_time_ticks_per_second = 1000000000;  // * nanoseconds

#define PRECISE_SECONDS         precise_time_ticks_per_second
#define PRECISE_MILLISECONDS    (precise_time_ticks_per_second / 1000ULL)
#define PRECISE_MICROSECONDS    (precise_time_ticks_per_second / 1000000ULL)
#define PRECISE_NANOSECONDS     (precise_time_ticks_per_second / 1000000000ULL)

#define NO_TIMEOUT              PRECISE_TIME_MAX

static inline uint64_t precise_time_to_milliseconds(precise_time_t time)
{
    return time / PRECISE_MILLISECONDS;
}

void ksleep(precise_time_t time);
