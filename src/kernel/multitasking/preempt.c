#include "preempt.h"
#include <stdatomic.h>

int _Atomic preempt_disable_depth = 0;

void recursive_preempt_disable()
{
    atomic_fetch_add(&preempt_disable_depth, 1);
}
void recursive_preempt_enable()
{
    // ! do NOT check for queued task switches here, would nullify the advantages of RCU
    atomic_fetch_sub(&preempt_disable_depth, 1);
}
