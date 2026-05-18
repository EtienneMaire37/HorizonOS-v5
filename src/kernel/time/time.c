#include "time.h"
#include "../util/vector.h"

atomic_flag time_lock = ATOMIC_FLAG_INIT;

struct timespec current_time;
int64_t tsc_remainder = 0;

bool time_initialized = false;

DEFINE_VECTOR(u64, uint64_t)

vector_u64_t sleep_queue = vector_u64_init;

bool tpause_supported = false;

#include "../cpu/util.h"
#include "../cpu/tsc.h"

void ksleep(precise_time_t time)
{
    // * Should guarantee to wait AT LEAST time
    uint64_t deadline = rdtsc() + time * tsc_cycles_per_second / PRECISE_SECONDS;
    if (tpause_supported)
    {
        tpause(1, deadline);
    }
    else
    {
        while (rdtsc() < deadline)
            __builtin_ia32_pause();
    }
}
