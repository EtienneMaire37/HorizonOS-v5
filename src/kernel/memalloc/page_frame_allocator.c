#pragma once

#include "page_frame_allocator.h"

void pfa_detect_usable_memory() 
{
    usable_memory = usable_memory_blocks = 0;
    bitmap = NULL;
    memory_allocated = 0;
    allocatable_memory = 0;
    first_free_page_index_hint = 0;

    LOG(INFO, "Usable memory map:");

    for (MMapEnt* mmap_ent = &bootboot.mmap; (uintptr_t)mmap_ent < (uintptr_t)&bootboot + (uintptr_t)bootboot.size; mmap_ent++) 
    {
        LOG(DEBUG, "   BOOTBOOT memory block : address : %#llx ; length : %llu | type : %u", 
            MMapEnt_Ptr(mmap_ent), MMapEnt_Size(mmap_ent), MMapEnt_Type(mmap_ent));
        // printf("   BOOTBOOT memory block : address : %#llx ; length : %llu | type : %u\n", 
        //     MMapEnt_Ptr(mmap_ent), MMapEnt_Size(mmap_ent), MMapEnt_Type(mmap_ent));

        if (!MMapEnt_IsFree(mmap_ent))
            continue;
        if (MMapEnt_Ptr(mmap_ent) == physical_null)
            continue;

        physical_address_t addr = MMapEnt_Ptr(mmap_ent);
        uint64_t len = MMapEnt_Size(mmap_ent);

        if (addr >= MAX_MEMORY)
            break;
        if (addr + len > MAX_MEMORY)
            len = MAX_MEMORY - addr;

        uint32_t align = addr & 0xfff;
        if (align != 0) 
        {
            uint32_t adjust = 0x1000 - align;
            addr += adjust;
            if (len < adjust)
                continue;
            len -= adjust;
        }

        len = (len / 0x1000) * 0x1000;
        if (len == 0)
            continue;

        if (usable_memory_blocks >= MAX_USABLE_MEMORY_BLOCKS) 
        {
            LOG(WARNING, "Too many memory blocks detected");
            break;
        }

        usable_memory_map[usable_memory_blocks].address = addr;
        usable_memory_map[usable_memory_blocks].length = (uint64_t)len;
        usable_memory += len;
        usable_memory_blocks++;

        LOG(INFO, "   Memory block : address : %#llx ; length : %llu", addr, len);
        // printf("   Memory block : address : %#llx ; length : %llu\n", addr, len);
    }

    first_alloc_block = 0;
    uint64_t total_pages = 0;
    for (uint64_t i = 0; i < usable_memory_blocks; i++)
        total_pages += usable_memory_map[i].length / 0x1000;
    
    bitmap_size = ((total_pages + 7) / 8 + 7) & ~7ULL; // * Align to qwords
    uint64_t bitmap_pages = (bitmap_size + 0xfff) / 0x1000;

    while (first_alloc_block < usable_memory_blocks && (usable_memory_map[first_alloc_block].length / 0x1000) < bitmap_pages)
        first_alloc_block++;

    bitmap = (uint8_t*)usable_memory_map[first_alloc_block].address;

    printf("pfa: bitmap address: %#llx\n", bitmap);

    if (usable_memory == 0 || first_alloc_block >= usable_memory_blocks) 
        goto no_memory;

    usable_memory_map[first_alloc_block].length -= 0x1000 * bitmap_pages;
    usable_memory_map[first_alloc_block].address += 0x1000 * bitmap_pages;
    if (usable_memory_map[first_alloc_block].length == 0)
        first_alloc_block++;

    if (first_alloc_block >= usable_memory_blocks)
        goto no_memory;

    memset(bitmap, 0, bitmap_size);

    for (uint8_t i = first_alloc_block; i < usable_memory_blocks; i++)
        allocatable_memory += usable_memory_map[i].length;

    LOG(INFO, "Detected %llu bytes of allocatable memory", allocatable_memory);
    return;

no_memory:
    LOG(CRITICAL, "No usable memory detected");
    abort();
}

static inline physical_address_t pfa_allocate_physical_page() 
{
    if (memory_allocated + 0x1000 > allocatable_memory) 
    {
        LOG(CRITICAL, "pfa_allocate_physical_page: Out of memory at start!");
        return physical_null;
    }

    if (memory_allocated + 0x1000 > allocatable_memory * 9 / 10) 
        LOG(WARNING, "pfa_allocate_physical_page: Over 90%% of memory used!");

    acquire_spinlock(&pfa_spinlock);

    for (uint64_t i = first_free_page_index_hint / 8; i < bitmap_size; i += 8) 
    {
        uint64_t* qword = (uint64_t*)&bitmap[i];
        if (*qword == 0xffffffffffffffff) continue;

        uint8_t bit = __builtin_ffsll(~(*qword)) - 1;

        *qword |= ((uint64_t)1 << bit);
        memory_allocated += 0x1000;

        uint64_t page_index = i * 8 + bit;

        uint64_t remaining = page_index;
        for (uint32_t j = first_alloc_block; j < usable_memory_blocks; j++) 
        {
            uint64_t block_pages = usable_memory_map[j].length / 0x1000;
            if (remaining < block_pages) 
            {
                physical_address_t addr = usable_memory_map[j].address + remaining * 0x1000;
                first_free_page_index_hint = 8 * i + bit;
                LOG_MEM_ALLOCATED();
                release_spinlock(&pfa_spinlock);
                // LOG(TRACE, "Allocated page: %#llx", addr);
                return addr;
            }
            remaining -= block_pages;
        }
    }

    LOG(CRITICAL, "pfa_allocate_physical_page: Out of memory at end!");
    release_spinlock(&pfa_spinlock);
    return physical_null;
}

static inline physical_address_t pfa_allocate_physical_contiguous_pages(uint32_t pages)
{
    if (pages != 1) abort();
    return pfa_allocate_physical_page();
}

static inline void pfa_free_physical_page(physical_address_t address) 
{
    if (address == physical_null) 
    {
        LOG(WARNING, "pfa_free_physical_page: Kernel tried to free NULL");
        return;
    }

    if (address & 0xfff) 
    {
        LOG(CRITICAL, "pfa_free_physical_page: Unaligned address (%#llx)", address);
        abort();
    }

    uint64_t page_index = 0;
    for (uint32_t i = first_alloc_block; i < usable_memory_blocks; i++) 
    {
        if (address >= usable_memory_map[i].address && 
            address < usable_memory_map[i].address + usable_memory_map[i].length) 
        {
            page_index += (address - usable_memory_map[i].address) / 0x1000;
            break;
        }
        page_index += usable_memory_map[i].length / 0x1000;
    }

    uint64_t byte = page_index / 8;
    uint8_t bit = page_index & 0b111;
    if (byte >= bitmap_size) return;
    acquire_spinlock(&pfa_spinlock);
    bitmap[byte] &= ~(1 << bit);
    if (page_index < first_free_page_index_hint)
        first_free_page_index_hint = page_index;

    memory_allocated -= 0x1000;
    release_spinlock(&pfa_spinlock);
    // LOG(TRACE, "Freed page: %#llx", address);
    LOG_MEM_ALLOCATED();
}

static inline void pfa_free_physical_contiguous_pages(physical_address_t address, uint32_t pages)
{
    if (pages != 1) abort();
    for (uint32_t i = 0; i < pages; i++)
        pfa_free_physical_page(address + 0x1000ULL * i);
}

// * 1st TB will always be identity mapped
static inline void* pfa_allocate_page()
{
    return (void*)pfa_allocate_physical_page();
}

// * same
static inline void pfa_free_page(const void* ptr)
{
    pfa_free_physical_page((physical_address_t)ptr);
}

// * same
static inline void* pfa_allocate_contiguous_pages(uint32_t pages)
{
    return (void*)pfa_allocate_physical_contiguous_pages(pages);
}

// * same
static inline void pfa_free_contiguous_pages(const void* ptr, uint32_t pages)
{
    pfa_free_physical_contiguous_pages((physical_address_t)ptr, pages);
}