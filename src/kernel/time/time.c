#include "time.h"
#include "../util/vector.h"

atomic_flag time_lock = ATOMIC_FLAG_INIT;
atomic_flag sleep_lock = ATOMIC_FLAG_INIT;

int64_t system_seconds = 0, system_minutes = 0, system_hours = 0, system_day = 0, system_month = 0;
int64_t system_year = 0;
int64_t system_milliseconds = 0;
int64_t tsc_remainder = 0;

bool time_initialized = false;

DEFINE_VECTOR(u64, uint64_t)

vector_u64_t sleep_queue = vector_u64_init;

#include "../util/error.h"

void ksleep(precise_time_t time)
{
    FATAL("Not implemented");
    // * Should guarantee to wait AT LEAST time
    acquire_spinlock(&sleep_lock);
    // ...
    release_spinlock(&sleep_lock);
}
