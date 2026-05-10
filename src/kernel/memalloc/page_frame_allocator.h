#pragma once

#include <stdint.h>
#include "../cpu/util.h"
#include "mmap.h"
#include "../multitasking/mutex.h"

#include <stdatomic.h>

#define MAX_MEMORY (128 * TB)

#define memory_map_get_free_pages(i)                (usable_memory_map[i].total_pages - usable_memory_map[i].used_pages)
#define memory_map_set_free_pages(i, free_pages)    usable_memory_map[i].used_pages = usable_memory_map[i].total_pages - (free_pages)

#define pfa_bitmap_get_page(i)                      (!!(bitmap[i / 8] & (1 << (i % 8))))
#define pfa_bitmap_set_page(i, sta)                 bitmap[i / 8] = (bitmap[i / 8] & ~(1 << (i % 8))) | (!!(sta) << (i % 8))

extern uint64_t usable_memory;

extern physical_address_t max_allocatable_address;

extern struct mem_block usable_memory_map[MAX_USABLE_MEMORY_BLOCKS];
extern uint8_t usable_memory_blocks;

extern uint64_t memory_allocated, allocatable_memory;

#define DO_LOG_MEM_ALLOCATED()      do { uint32_t percentage = 10000 * memory_allocated / allocatable_memory; LOG(INFO, "Used memory : %" PRIu64 " / %" PRIu64 " bytes (%u.%u%u %%)", memory_allocated, allocatable_memory, percentage / 100, (percentage / 10) % 10, percentage % 10); } while (0)

#ifdef LOG_MEMORY
#define LOG_MEM_ALLOCATED() DO_LOG_MEM_ALLOCATED()
#else
#define LOG_MEM_ALLOCATED()
#endif

void pfa_detect_usable_memory();
physical_address_t pfa_allocate_physical_page();
physical_address_t pfa_allocate_physical_contiguous_pages(size_t pages);
void pfa_free_physical_page(physical_address_t address);
void pfa_free_page(const void* ptr);
void* __attribute__((malloc, malloc(pfa_free_page, 1), assume_aligned(4096))) pfa_allocate_page();
void pfa_free_contiguous_pages(const void* ptr, size_t pages);
void* __attribute__((malloc, malloc(pfa_free_contiguous_pages, 1), assume_aligned(4096))) pfa_allocate_contiguous_pages(size_t pages);
