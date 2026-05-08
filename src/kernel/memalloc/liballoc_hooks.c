#include "page_frame_allocator.h"
#include "virtual_memory_allocator.h"
#include "../multitasking/mutex.h"
#include "../multitasking/multitasking.h"

// ! Should be per cpu core
uint32_t liballoc_flags;

spinlock_noint_t liballoc_spinlock = SPINLOCK_NOINT_INIT;

int liballoc_lock()
{
    liballoc_flags = acquire_spinlock_noint(&liballoc_spinlock);
	return 0;
}

int liballoc_unlock()
{
    release_spinlock_noint(&liballoc_spinlock, liballoc_flags);
	return 0;
}

void* liballoc_alloc(size_t pages)
{
    uint32_t flags = acquire_spinlock_noint(&vmm_lock);
	void* addr = __vmm_find_free_kernel_space_pages(NULL, pages);
    // LOG(TRACE, "liballoc_alloc: %p", addr);
	if (unlikely(!addr)) return (release_spinlock_noint(&vmm_lock, flags), NULL);
	allocate_range((uint64_t*)(get_cr3_address() + PHYS_MAP_BASE), (uint64_t)addr, pages, PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);
	release_spinlock_noint(&vmm_lock, flags);
    return addr;
}

int liballoc_free(void* ptr, size_t pages)
{
	free_range((uint64_t*)(get_cr3_address() + PHYS_MAP_BASE), (uint64_t)ptr, pages);
    return 0;
}
