#include "rcu.h"
#include "../multitasking/preempt.h"

void rcu_read_lock()
{
    recursive_preempt_disable();
}
void rcu_read_unlock()
{
    recursive_preempt_enable();
}

void synchronize_rcu()
{
    // TODO: Assert to check if not in interrupt handler
}
