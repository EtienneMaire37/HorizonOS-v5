#include "preempt.h"
#include "multitasking.h"
#include <stdatomic.h>

// * Per CPU
int _Atomic preempt_disable_depth = 0;

void recursive_preempt_disable()
{
    atomic_fetch_add(&preempt_disable_depth, 1);
}
void recursive_preempt_enable()
{
    atomic_fetch_sub(&preempt_disable_depth, 1);
    if (queued_ts && multitasking_enabled)
    {
        queued_ts = false;
        switch_task();
    }
}
