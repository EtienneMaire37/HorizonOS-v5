#pragma once

#include "../paging/paging.h"

physical_address_t vas_create_empty()
{
    physical_address_t cr3 = pfa_allocate_physical_page();
    physical_init_page_directory(cr3);
    physical_add_page_table(cr3, 1023, cr3, PAGING_SUPERVISOR_LEVEL, true); // recursive paging

    physical_add_page_table(cr3, 0, pfa_allocate_physical_page(), PAGING_SUPERVISOR_LEVEL, true);

    physical_address_t first_mb_pt_address = read_physical_address_4b(cr3) & 0xfffff000;
    for (uint16_t i = 0; i < 256; i++)
        physical_set_page(first_mb_pt_address, i, i * 0x1000, PAGING_SUPERVISOR_LEVEL, true);

    for (uint16_t i = 0; i < 255; i++)  // !!!!! DONT OVERRIDE RECURSIVE PAGING
    {
        physical_add_page_table(cr3, i + 768, pfa_allocate_physical_page(), PAGING_SUPERVISOR_LEVEL, true);

        struct virtual_address_layout layout;
        layout.page_directory_entry = i + 768;
        layout.page_offset = 0;
        physical_address_t pt_address = read_physical_address_4b(cr3 + 4 * layout.page_directory_entry) & 0xfffff000;
        for (uint16_t j = 0; j < 1024; j++)
        {
            layout.page_table_entry = j;
            uint32_t address = *(uint32_t*)&layout - 0xc0000000;
            physical_set_page(pt_address, layout.page_table_entry, address, PAGING_SUPERVISOR_LEVEL, true);
        }
    }

    // * Stacks and physical memory access
    physical_add_page_table(cr3, 767, pfa_allocate_physical_page(), PAGING_SUPERVISOR_LEVEL, true);   

    physical_address_t pt_address = read_physical_address_4b(cr3 + 4 * 767) & 0xfffff000;
    physical_set_page(pt_address, physical_memory_page_index, 0, PAGING_SUPERVISOR_LEVEL, true);

    physical_set_page(pt_address, kernel_stack_page_index, pfa_allocate_physical_page(), PAGING_SUPERVISOR_LEVEL, true);

    for (int i = stack_page_index_start; i <= stack_page_index_end; i++)
        physical_set_page(pt_address, i, pfa_allocate_physical_page(), PAGING_USER_LEVEL, true);
        
    return cr3;
}

void vas_free(physical_address_t cr3)
{
    assert(("cr3 is not page aligned", (cr3 & 0xfff) == 0));

    if (cr3 == physical_null) return;

    for (uint16_t i = 0; i < 768; i++)
    {
        uint32_t pde = read_physical_address_4b(cr3 + (i * 4));
        if (!(pde & 1)) continue;
        uint32_t pt_address = pde & 0xfffff000;
        for (uint16_t j = 0; j < 1024; j++)
        {
            if (i == 767 && j == physical_memory_page_index) continue;
            uint32_t pte = read_physical_address_4b(pt_address + (j * 4));
            if (!(pte & 1)) continue;
            uint32_t page_address = pte & 0xfffff000;
            pfa_free_physical_page(page_address);
        }
        pfa_free_physical_page(pt_address);
    }

    pfa_free_physical_page(cr3);
}