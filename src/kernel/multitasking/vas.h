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
    uint64_t* cr3 = (uint64_t*)pml4_address;
// * Skip kernel mappings
    for (uint16_t pml4e = 2; pml4e < 511; pml4e++)
    {
        if (!is_pdpt_entry_present(&cr3[pml4e])) continue;

        uint64_t* pdpt_address = get_pdpt_entry_address(&cr3[pml4e]);
        for (uint16_t pdpte = 0; pdpte < 512; pdpte++)
        {
            if (!is_pdpt_entry_present(&pdpt_address[pdpte])) continue;

            uint64_t* pd_address = get_pdpt_entry_address(&pdpt_address[pdpte]);
            for (uint16_t pde = 0; pde < 512; pde++)
            {
                if (!is_pdpt_entry_present(&pd_address[pde])) continue;

                uint64_t* pt_address = get_pdpt_entry_address(&pd_address[pde]);
                for (uint16_t pte = 0; pte < 512; pte++)
                {
                    if (!is_pdpt_entry_present(&pt_address[pte])) continue;

                    pfa_free_page(get_pdpt_entry_address(&pt_address[pte]));
                }
                pfa_free_page(pt_address);
            }
            pfa_free_page(pd_address);
        }
        pfa_free_page(pdpt_address);
    }
    pfa_free_physical_page(pml4_address);
}