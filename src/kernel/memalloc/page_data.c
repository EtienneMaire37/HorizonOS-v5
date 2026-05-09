#include "page_data.h"
#include "page_frame_allocator.h"
#include "../paging/paging.h"
#include <assert.h>
#include <stdlib.h>

page_table_data_t* global_page_table = NULL;

void page_data_table_init()
{
    assert(!global_page_table);
    size_t npages = ((sizeof(page_table_data_t) * allocatable_memory) + 0xffffff) / 0x1000000;
    global_page_table = pfa_allocate_contiguous_pages(npages);
    LOG(DEBUG, "global_page_table = %p [%zu pages]", global_page_table, npages);
    assert(global_page_table);
}
uint32_t lock_page_table(uint64_t* addr)
{
    assert(global_page_table);
    return acquire_spinlock_noint(&global_page_table[((uint64_t)addr - PHYS_MAP_BASE) / 4096].lock);
}
void unlock_page_table(uint64_t* addr, uint32_t eflags)
{
    assert(global_page_table);
    release_spinlock_noint(&global_page_table[((uint64_t)addr - PHYS_MAP_BASE) / 4096].lock, eflags);
}
