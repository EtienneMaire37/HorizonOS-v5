#include "../debug/out.h"
#include "../cpu/units.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../paging/paging.h"
#include "mmap.h"
#include "page_frame_allocator.h"
#include "../boot/limine.h"
#include "../int/kernel_panic.h"

uint64_t usable_memory = 0;
struct mem_block usable_memory_map[MAX_USABLE_MEMORY_BLOCKS];
uint8_t usable_memory_blocks = 0;
physical_address_t max_allocatable_address = 0;

uint8_t first_alloc_block;

uint64_t bitmap_size;
uint8_t* bitmap;

uint64_t first_free_page_index_hint = 0;

uint64_t memory_allocated, allocatable_memory;

spinlock_noint_t pfa_lock = SPINLOCK_NOINT_INIT;

void pfa_detect_usable_memory()
{
    usable_memory = usable_memory_blocks = 0;
    bitmap = NULL;
    memory_allocated = 0;
    allocatable_memory = 0;
    first_free_page_index_hint = 0;

    LOG(INFO, "Usable memory map:");

    for (int i = 0; i < mmap_request.response->entry_count; i++)
    {
        struct limine_memmap_entry* entry = mmap_request.response->entries[i];
        LOG(DEBUG, "   Limine memory block : address : %#" PRIx64 " ; length : %" PRIu64 " | type : %" PRIu64,
            entry->base, entry->length, entry->type);
        if (entry->type != LIMINE_MEMMAP_USABLE)
            continue;
        if (entry->base & 0xfff)
            continue;
        if (entry->length & 0xfff)
            continue;
        assert(usable_memory_blocks < MAX_USABLE_MEMORY_BLOCKS);

        physical_address_t addr = entry->base;
        uint64_t len = entry->length;

        if (!addr) // * Skip NULL
        {
            addr += 0x1000;
            len -= 0x1000;
            if (len == 0)
                continue;
        }

        if (addr >= MAX_MEMORY)
            break;
        if (addr > MAX_MEMORY - len)
            len = MAX_MEMORY - addr;

        uint64_t align = addr & 0xfff;
        if (align != 0)
        {
            uint64_t adjust = 0x1000 - align;
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
        usable_memory_map[usable_memory_blocks].total_pages = len / 4096;
        usable_memory_map[usable_memory_blocks].used_pages = 0;
        usable_memory += len;
        usable_memory_blocks++;

        LOG(INFO, "   Memory block : address : %#" PRIx64 " ; length : %" PRIu64, addr, len);
        // printf("Memory block : address : %#" PRIx64 " ; length : %" PRIu64 "\n", addr, len);
    }

    first_alloc_block = 0;
    uint64_t total_pages = 0;
    for (uint64_t i = 0; i < usable_memory_blocks; i++)
        total_pages += usable_memory_map[i].total_pages;

    bitmap_size = total_pages / 8; // * Align to qwords (and round down)
    uint64_t bitmap_pages = bitmap_size / 0x1000;

    LOG(TRACE, "Bitmap size: %" PRIu64, bitmap_size);
    printf("PFA: Bitmap size: %" PRIu64 "\n", bitmap_size);

    while (first_alloc_block < usable_memory_blocks && usable_memory_map[first_alloc_block].total_pages <= bitmap_pages)
        first_alloc_block++;

    assert(first_alloc_block < usable_memory_blocks);

    bitmap = (uint8_t*)(usable_memory_map[first_alloc_block].address + PHYS_MAP_BASE);

    LOG(TRACE, "PFA: bitmap address: %p", bitmap);
    printf("PFA: bitmap address: %p\n", bitmap);

    if (first_alloc_block >= usable_memory_blocks)
    goto no_memory;

    if (usable_memory_map[first_alloc_block].total_pages <= bitmap_pages)
        first_alloc_block++;
    else
    {
        usable_memory_map[first_alloc_block].total_pages -= bitmap_pages;
        usable_memory_map[first_alloc_block].address += 0x1000 * bitmap_pages;
    }

    if (first_alloc_block >= usable_memory_blocks)
        goto no_memory;
    LOG(TRACE, "PFA: First alloc address: %#" PRIx64, usable_memory_map[first_alloc_block].address + PHYS_MAP_BASE);

    memset(bitmap, 0, bitmap_size);

    for (uint8_t i = first_alloc_block; i < usable_memory_blocks; i++)
        allocatable_memory += 0x1000 * usable_memory_map[i].total_pages;

    max_allocatable_address = usable_memory_map[usable_memory_blocks - 1].address + (usable_memory_map[usable_memory_blocks - 1].total_pages - 1) * 0x1000;

    LOG(INFO, "Detected %" PRIu64 " bytes of allocatable memory", allocatable_memory);
    return;

no_memory:
    LOG(CRITICAL, "Not enough usable memory detected");
    FATAL("Not enough usable memory detected");
}

physical_address_t pfa_allocate_physical_page()
{
    return pfa_allocate_physical_contiguous_pages(1);
}

physical_address_t pfa_allocate_physical_contiguous_pages(size_t pages)
{
    // LOG(TRACE, "pfa_allocate_physical_contiguous_pages(%zu)", pages);
    if (pages == 0)
        return physical_null;

    if (memory_allocated + 0x1000 * pages > allocatable_memory)
    {
        LOG(CRITICAL, "pfa_allocate_physical_contiguous_pages: Out of memory at start! (%zu pages were asked while %zu are free)", pages, (allocatable_memory - memory_allocated) / 0x1000);
        kernel_panic_ex(NULL, PANIC_OUT_OF_MEMORY);
        abort();
    }

    uint32_t flags = acquire_spinlock_noint(&pfa_lock);

    uint64_t block_start_index = 0;
    for (uint32_t i = first_alloc_block; i < usable_memory_blocks; i++)
    {
        if (memory_map_get_free_pages(i) >= pages)
        {
            size_t contiguous_pages = 0;
            uint64_t alloc_start = block_start_index;
            for (uint32_t j = 0; j < usable_memory_map[i].total_pages; j++)
            {
                if (!pfa_bitmap_get_page(block_start_index + j))
                {
                    if (contiguous_pages == 0)
                        alloc_start = block_start_index + j;
                    contiguous_pages++;
                    if (contiguous_pages >= pages)
                    {
                        for (uint64_t k = 0; k < pages; k++)
                            pfa_bitmap_set_page(alloc_start + k, 1);
                        usable_memory_map[i].used_pages++;
                        memory_allocated += 0x1000 * pages;
                        release_spinlock_noint(&pfa_lock, flags);
                        return usable_memory_map[i].address + j * 0x1000ULL;
                    }
                }
                else
                    contiguous_pages = 0;
            }
        }
        block_start_index += usable_memory_map[i].total_pages;
    }

    LOG(CRITICAL, "pfa_allocate_physical_contiguous_pages: Out of memory at end!");
    kernel_panic_ex(NULL, PANIC_OUT_OF_MEMORY);
    abort();
}

void pfa_free_physical_page(physical_address_t address)
{
    if (address == physical_null)
    {
        LOG(WARNING, "pfa_free_physical_page: Kernel tried to free NULL");
        return;
    }

    assert(!(address & 0xfff));

    #ifdef DEBUG_ALLOCATOR
    return;
    #endif

    uint64_t page_index = 0;
    for (uint32_t i = first_alloc_block; i < usable_memory_blocks; i++)
    {
        if (address >= usable_memory_map[i].address &&
            address < usable_memory_map[i].address + 0x1000 * usable_memory_map[i].total_pages)
        {
            page_index += (address - usable_memory_map[i].address) / 0x1000;
            usable_memory_map[i].used_pages--;
            break;
        }
        if (i == usable_memory_blocks - 1)
            return;
        page_index += usable_memory_map[i].total_pages;
    }

    uint32_t flags = acquire_spinlock_noint(&pfa_lock);

    pfa_bitmap_set_page(page_index, 0);

    memory_allocated -= 0x1000;
    release_spinlock_noint(&pfa_lock, flags);
    LOG_MEM_ALLOCATED();
}

void pfa_free_physical_contiguous_pages(physical_address_t address, size_t pages)
{
    for (size_t i = 0; i < pages; i++)
        pfa_free_physical_page(address + 0x1000ULL * i);
}

__attribute__((malloc, malloc(pfa_free_page, 1), assume_aligned(4096))) void* pfa_allocate_page()
{
    physical_address_t paddr = pfa_allocate_physical_page();
    if (!paddr) return NULL;
    return (void*)(paddr + PHYS_MAP_BASE);
}

void pfa_free_page(const void* ptr)
{
    if (!ptr) return;
    pfa_free_physical_page((physical_address_t)ptr - PHYS_MAP_BASE);
}

__attribute__((malloc, malloc(pfa_free_contiguous_pages, 1), assume_aligned(4096))) void* pfa_allocate_contiguous_pages(size_t pages)
{
    physical_address_t paddr = pfa_allocate_physical_contiguous_pages(pages);
    if (!paddr) return NULL;
    return (void*)(paddr + PHYS_MAP_BASE);
}

void pfa_free_contiguous_pages(const void* ptr, size_t pages)
{
    if (!ptr) return;
    pfa_free_physical_contiguous_pages((physical_address_t)ptr - PHYS_MAP_BASE, pages);
}
