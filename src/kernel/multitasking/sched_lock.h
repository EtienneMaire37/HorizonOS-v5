#pragma once

#include "../multicore/spinlock.h"

extern spinlock_noint_t sched_lock;

#define try_lock_scheduler(flags)   try_acquire_spinlock_noint(&sched_lock, flags)
#define lock_scheduler()            ({ /* LOG(TRACE, "Locking scheduler from %s.%d", __FILE__, __LINE__); */ acquire_spinlock_noint(&sched_lock); })
void unlock_scheduler(uint32_t flags);
