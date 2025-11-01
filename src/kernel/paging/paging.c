#pragma once

#include "../cpu/memory.h"

void* create_empty_virtual_address_space()
{
    uint32_t* pml4 = pfa_allocate_page();
    if (!pml4) 
    {
        LOG(ERROR, "Coudln't create PML4!!!");
        return NULL;
    }

    for (int i = 0; i < 512; i++)
        pml4[i] = 0;    // non present

    return (void*)pml4;
}