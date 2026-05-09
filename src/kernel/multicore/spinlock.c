#include "spinlock.h"
#include "../multitasking/multitasking.h"

bool try_acquire_spinlock_noint(atomic_flag* spinlock, uint32_t* flags)
{
    uint32_t eflags = get_rflags();
    *flags = eflags;
    disable_interrupts();
    if (eflags & (1 << 9))
    {
        if (try_acquire_spinlock(spinlock))
        {
            enable_interrupts();
            return true;
        }
        enable_interrupts();
        return false;
    }
    return try_acquire_spinlock(spinlock);
}

uint32_t acquire_spinlock_noint(atomic_flag* spinlock)
{
    uint32_t eflags = get_rflags();
    disable_interrupts();
    while (try_acquire_spinlock(spinlock))
    {
        FATAL("DEADLOCK");
        set_rflags_if(eflags);
        __builtin_ia32_pause();
        disable_interrupts();
    }
    return eflags;
}

void release_spinlock_noint(atomic_flag* spinlock, uint32_t eflags)
{
	atomic_flag_clear_explicit(spinlock, memory_order_release);
	set_rflags_if(eflags);
}
