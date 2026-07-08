#include "vas.h"
#include "multitasking.h"
#include "../memalloc/page_data.h"

physical_address_t task_create_empty_vas(uint8_t privilege)
{
    physical_address_t paddr = create_empty_pdpt_phys();
    uint64_t* cr3 = (uint64_t*)(paddr + PHYS_MAP_BASE);
    if (!paddr) return physical_null;

    for (int i = 256; i < 512; i++)
        set_pdpt_entry(&cr3[i], get_pdpt_entry_address(&global_cr3[i]), PG_USER, PG_READ_WRITE, CACHE_WB);

    allocate_range(cr3, TASK_STACK_BOTTOM_ADDRESS, TASK_STACK_PAGES,
            privilege, PG_READ_WRITE, CACHE_WB);
    if (privilege != PG_SUPERVISOR)
        allocate_range(cr3, TASK_KERNEL_STACK_BOTTOM_ADDRESS, TASK_KERNEL_STACK_PAGES,
            PG_USER, PG_READ_WRITE, CACHE_WB);
    return paddr;
}

void task_free_vas(physical_address_t cr3)
{
    uint64_t* pml4_address = (uint64_t*)(cr3 + PHYS_MAP_BASE);
    uint32_t pml4_flags = lock_page_table(pml4_address);
    for (uint16_t pml4e = 0; pml4e < 256; pml4e++)
    {
        if (!is_pdpt_entry_present(&pml4_address[pml4e])) continue;

        uint64_t* pdpt_address = (uint64_t*)(PHYS_MAP_BASE + get_pdpt_entry_address(&pml4_address[pml4e]));
        uint32_t pdpt_flags = lock_page_table(pdpt_address);
        for (uint16_t pdpte = 0; pdpte < 512; pdpte++)
        {
            if (!is_pdpt_entry_present(&pdpt_address[pdpte])) continue;

            uint64_t* pd_address = (uint64_t*)(PHYS_MAP_BASE + get_pdpt_entry_address(&pdpt_address[pdpte]));
            uint32_t pd_flags = lock_page_table(pd_address);
            for (uint16_t pde = 0; pde < 512; pde++)
            {
                if (!is_pdpt_entry_present(&pd_address[pde])) continue;

                uint64_t* pt_address = (uint64_t*)(PHYS_MAP_BASE + get_pdpt_entry_address(&pd_address[pde]));
                uint32_t pt_flags = lock_page_table(pt_address);
                for (uint16_t pte = 0; pte < 512; pte++)
                {
                    if (!is_pdpt_entry_present(&pt_address[pte])) continue;

                    pfa_free_physical_page(get_pdpt_entry_address(&pt_address[pte]));
                }
                unlock_page_table(pt_address, pt_flags);
                pfa_free_physical_page(get_pdpt_entry_address(&pd_address[pde]));
            }
            unlock_page_table(pd_address, pd_flags);
            pfa_free_physical_page(get_pdpt_entry_address(&pdpt_address[pdpte]));
        }
        unlock_page_table(pdpt_address, pdpt_flags);
        pfa_free_physical_page(get_pdpt_entry_address(&pml4_address[pml4e]));
    }
    unlock_page_table(pml4_address, pml4_flags);
    pfa_free_physical_page(cr3);
}
