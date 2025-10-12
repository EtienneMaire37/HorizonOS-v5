#pragma once

inline void acquire_spinlock(atomic_flag* spinlock)
{
	while (atomic_flag_test_and_set_explicit(spinlock, memory_order_acquire))
        __builtin_ia32_pause();
}

inline void release_spinlock(atomic_flag* spinlock)
{
	atomic_flag_clear_explicit(spinlock, memory_order_release);
}