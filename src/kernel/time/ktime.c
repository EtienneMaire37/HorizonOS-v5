#include "ktime.h"
#include "time.h"

#include "../cpu/tsc.h"

// * Assumes time_lock is acquired
void __resolve_time()
{
    if (tsc_cycles_per_second)
    {
        uint64_t new_tsc = rdtsc();
        uint64_t tsc_diff = new_tsc - last_tsc;
        last_tsc = new_tsc;
        tsc_remainder += tsc_diff;

        uint64_t seconds_inc = tsc_remainder / tsc_cycles_per_second;
        uint64_t cycles_left = tsc_remainder % tsc_cycles_per_second;

        uint64_t fractional_nsec = (cycles_left * 1000000000ULL) / tsc_cycles_per_second;

        current_time.tv_sec += seconds_inc;
        current_time.tv_nsec += fractional_nsec;

        tsc_remainder = cycles_left - ((fractional_nsec * tsc_cycles_per_second) / 1000000000ULL);

        if (current_time.tv_nsec >= 1000000000)
        {
            current_time.tv_sec += current_time.tv_nsec / 1000000000;
            current_time.tv_nsec %= 1000000000;
        }
    }
}
