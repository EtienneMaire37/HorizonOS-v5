#pragma once

#include "../paging/paging.h"
#include "task.h"

physical_address_t task_create_empty_vas(uint8_t privilege)
{
    uint64_t* cr3 = create_empty_pdpt();
    if (!cr3) return physical_null;

    set_pdpt_entry(&cr3[0], (uint64_t)get_pdpt_entry_address(&global_cr3[0]), PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);
    set_pdpt_entry(&cr3[1], (uint64_t)get_pdpt_entry_address(&global_cr3[1]), PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);

    set_pdpt_entry(&cr3[511], (uint64_t)get_pdpt_entry_address(&global_cr3[511]), PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);

    allocate_range(cr3, TASK_STACK_BOTTOM_ADDRESS, TASK_STACK_PAGES,
            privilege, PG_READ_WRITE, CACHE_WB);

    return (physical_address_t)cr3;
}

void task_free_vas(physical_address_t pml4_address)
{
    
}