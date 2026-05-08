#include "page_data.h"
#include "page_frame_allocator.h"
#include "../paging/paging.h"
#include <assert.h>
#include <stdlib.h>

page_table_data_t* global_page_table = NULL;

void page_data_table_init()
{
    assert(!global_page_table);
    global_page_table = malloc(sizeof(page_table_data_t) * allocatable_memory / 0x1000);
    assert(global_page_table);
}
uint32_t lock_page_table(uint64_t* addr)
{
    return acquire_spinlock_noint(&global_page_table[((uint64_t)addr - PHYS_MAP_BASE) / 4096].lock);
}
void unlock_page_table(uint64_t* addr, uint32_t eflags)
{
    release_spinlock_noint(&global_page_table[((uint64_t)addr - PHYS_MAP_BASE) / 4096].lock, eflags);
}
