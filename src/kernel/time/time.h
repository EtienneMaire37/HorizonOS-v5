#pragma once

typedef uint64_t precise_time_t;

#define GLOBAL_TIMER_FREQUENCY  1000
#define GLOBAL_TIMER_INCREMENT  (precise_time_ticks_per_second / GLOBAL_TIMER_FREQUENCY)

uint64_t global_timer = 0;

const int precise_time_ticks_per_second = 1000000;  // * microseconds

uint64_t precise_time_to_milliseconds(precise_time_t time)
{
    return time * 1000 / precise_time_ticks_per_second;
}

#define PRECISE_SECONDS         precise_time_ticks_per_second
#define PRECISE_MILLISECONDS    (precise_time_ticks_per_second / 1000)
#define PRECISE_MICROSECONDS    (precise_time_ticks_per_second / 1000000)