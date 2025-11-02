#pragma once

#include "../cpu/memory.h"
#include "paging.h"

static inline uint64_t* create_empty_pdpt()
{
    uint64_t* pdpt = pfa_allocate_page();
    if (!pdpt) 
    {
        LOG(ERROR, "Couldn't create PDPT!!!");
        return NULL;
    }

    // for (int i = 0; i < 512; i++)
    //     pdpt[i] = 0;    // non present

    memset(pdpt, 0, 4096);

    return (uint64_t*)pdpt;
}

uint64_t* create_empty_virtual_address_space()
{
    return create_empty_pdpt();
}

static inline bool is_pdpt_entry_present(const uint64_t* entry)
{
    return (*entry) & 1;
}

static inline uint64_t* get_pdpt_entry_address(const uint64_t* entry)
{
    return is_pdpt_entry_present(entry) ? (uint64_t*)(((*entry) & 0xfffffffffffff000) & get_physical_address_mask()) : NULL;
}

static inline void remove_pdpt_entry(uint64_t* entry)
{
    *entry = 0;
}

static inline void set_pdpt_entry(uint64_t* entry, uint64_t address, uint8_t privilege, uint8_t read_write, uint8_t cache_type)
{
    if (address == 0)
    {
        remove_pdpt_entry(entry);
        return;
    }

    cache_type &= 7;

    uint64_t pcd_bit = (pdpt_pat_bits[cache_type] & 3) >> 1,
             pwt_bit = (pdpt_pat_bits[cache_type] & 3) & 1;

    uint64_t masked_address = (address & 0xfffffffffffff000) & get_physical_address_mask();
    if (masked_address != address)
    {
        LOG(CRITICAL, "Kernel tried to map physical address 0x%x but it doesn't fit in %u bits", address, physical_address_width);
        abort();
    }

    *entry = masked_address | (pcd_bit << 4) | (pwt_bit << 3) | ((privilege & 1) << 2) | ((read_write & 1) << 1) | 1;
}

void remap_range(uint64_t* pml4, 
    uint64_t start_virtual_address, uint64_t start_physical_address, 
    uint64_t pages,
    uint8_t privilege, uint8_t read_write, uint8_t cache_type)
{
    if (!pml4)
    {
        LOG(ERROR, "remap_range: NULL virtual address space");
        return;
    }

    if ((start_virtual_address | start_physical_address) & 0xfff)
    {
        LOG(CRITICAL, "remap_range: Kernel tried to map non page aligned addresses");
        abort();
    }

    // uint64_t end_virtual_address = start_virtual_address + 0x1000 * pages;

    // for (uint64_t vaddr = start_virtual_address; vaddr < end_virtual_address; vaddr += 0x1000)
    for (uint64_t i = 0; i < pages; i++)
    {
        uint64_t vaddr = start_virtual_address + 0x1000 * i;

        uint64_t pte = (vaddr >> 12) & 0x1ff;
        uint64_t pde = (vaddr >> (12 + 9)) & 0x1ff;
        uint64_t pdpte = (vaddr >> (12 + 2 * 9)) & 0x1ff;
        uint64_t pml4e = (vaddr >> (12 + 3 * 9)) & 0x1ff;

        uint64_t* pml4_entry = &pml4[pml4e];
        if (!is_pdpt_entry_present(pml4_entry))
            set_pdpt_entry(pml4_entry, (uintptr_t)create_empty_pdpt(), PG_USER, PG_READ_WRITE, CACHE_WB);

        uint64_t* pdpt = get_pdpt_entry_address(pml4_entry);
        
        uint64_t* pdpt_entry = &pdpt[pdpte];
        if (!is_pdpt_entry_present(pdpt_entry))
            set_pdpt_entry(pdpt_entry, (uintptr_t)create_empty_pdpt(), PG_USER, PG_READ_WRITE, CACHE_WB);

        uint64_t* pd = get_pdpt_entry_address(pdpt_entry);

        uint64_t* pd_entry = &pd[pde];
        if (!is_pdpt_entry_present(pd_entry))
            set_pdpt_entry(pd_entry, (uintptr_t)create_empty_pdpt(), PG_USER, PG_READ_WRITE, CACHE_WB);

        uint64_t* pt = get_pdpt_entry_address(pd_entry);

        uint64_t* pt_entry = &pt[pte];
        if (is_pdpt_entry_present(pt_entry))
        {
            continue;
        }

        set_pdpt_entry(pt_entry, vaddr - start_virtual_address + start_physical_address, 
            privilege, read_write,
            cache_type);
    }
}

void copy_mapping(uint64_t* src, uint64_t* dst, 
    uint64_t start_virtual_address, 
    uint64_t pages)
{
    if (!src)
    {
        LOG(ERROR, "copy_mapping: NULL source virtual address space");
        return;
    }

    if (!dst)
    {
        LOG(ERROR, "copy_mapping: NULL destination virtual address space");
        return;
    }

    if (start_virtual_address & 0xfff)
    {
        LOG(CRITICAL, "copy_mapping: Kernel tried to map non page aligned addresses");
        abort();
    }

    for (uint64_t i = 0; i < pages; i++)
    {
        uint64_t vaddr = start_virtual_address + 0x1000 * i;

        if (vaddr < start_virtual_address)
            break;

        uint64_t pte = (vaddr >> 12) & 0x1ff;
        uint64_t pde = (vaddr >> (12 + 9)) & 0x1ff;
        uint64_t pdpte = (vaddr >> (12 + 2 * 9)) & 0x1ff;
        uint64_t pml4e = (vaddr >> (12 + 3 * 9)) & 0x1ff;

        uint64_t* old_pml4_entry = &src[pml4e];
        if (!is_pdpt_entry_present(old_pml4_entry))
        {
            i += ((uint64_t)1 << (9 * 3)) - (pdpte << (9 * 2)) - (pde << 9) - pte - 1;
            continue;
        }

        uint64_t* new_pml4_entry = &dst[pml4e];

        if (!is_pdpt_entry_present(new_pml4_entry))
            set_pdpt_entry(new_pml4_entry, (uintptr_t)create_empty_pdpt(), PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);

        uint64_t* old_pdpt = get_pdpt_entry_address(old_pml4_entry);
        
        uint64_t* old_pdpt_entry = &old_pdpt[pdpte];
        if (!is_pdpt_entry_present(old_pdpt_entry))
        {
            i += ((uint64_t)1 << (9 * 2)) - (pde << 9) - pte - 1;
            continue;
        }

        uint64_t* new_pdpt = get_pdpt_entry_address(new_pml4_entry);

        uint64_t* new_pdpt_entry = &new_pdpt[pdpte];

        if (!is_pdpt_entry_present(new_pdpt_entry))
            set_pdpt_entry(new_pdpt_entry, (uintptr_t)create_empty_pdpt(), PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);

        uint64_t* old_pd = get_pdpt_entry_address(old_pdpt_entry);

        uint64_t* old_pd_entry = &old_pd[pde];
        if (!is_pdpt_entry_present(old_pd_entry))
        {
            i += ((uint64_t)1 << 9) - pte - 1;
            continue;
        }

        uint64_t* new_pd = get_pdpt_entry_address(new_pdpt_entry);

        uint64_t* new_pd_entry = &new_pd[pde];

        if (!is_pdpt_entry_present(new_pd_entry))
            set_pdpt_entry(new_pd_entry, (uintptr_t)create_empty_pdpt(), PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);

        uint64_t* old_pt = get_pdpt_entry_address(old_pd_entry);
        uint64_t* new_pt = get_pdpt_entry_address(new_pd_entry);

        memcpy(new_pt, old_pt, 4096);

        i += ((uint64_t)1 << 9) - pte - 1;
    }
}

void log_virtual_address_space(uint64_t* cr3, uint64_t start, uint64_t end)
{
    LOG(DEBUG, "Virtual address space 0x%x:", cr3);

    for (int i = 0; i < 512; i++)
    {
        if (!is_pdpt_entry_present(&cr3[i]))
            continue;
        
        uint64_t* pdpt = get_pdpt_entry_address(&cr3[i]);
        
        for (int j = 0; j < 512; j++)
        {
            if (!is_pdpt_entry_present(&pdpt[j]))
                continue;
            
            uint64_t* pd = get_pdpt_entry_address(&pdpt[j]);

            uint64_t vaddr = i * ((uint64_t)1 << (12 + 3 * 9)) + j * ((uint64_t)1 << (12 + 2 * 9));

            // LOG(DEBUG, "\tpd: 0x%x-0x%x", vaddr, vaddr + ((uint64_t)1 << (12 + 2 * 9)));

            for (int k = 0; k < 512; k++)
            {
                if (!is_pdpt_entry_present(&pd[k]))
                    continue;
                
                uint64_t* pt = get_pdpt_entry_address(&pd[k]);

                vaddr += k * ((uint64_t)1 << (12 + 1 * 9));

                for (int l = 0; l < 512; l++)
                {
                    if (!is_pdpt_entry_present(&pt[l]))
                        continue;
                    
                    uint64_t* page = get_pdpt_entry_address(&pt[l]);

                    vaddr += l * ((uint64_t)1 << 12);

                    if (vaddr >= start && vaddr < end)
                        LOG(DEBUG, "\tpage: 0x%x-0x%x", vaddr, vaddr + 0x1000);
                }
            }
        }
    }
}