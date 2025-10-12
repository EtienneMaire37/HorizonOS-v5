#pragma once

uint32_t usable_memory;

struct mem_block
{
    physical_address_t address;
    uint32_t length;
} __attribute__((packed));

#define MAX_USABLE_MEMORY_BLOCKS 64
struct mem_block usable_memory_map[MAX_USABLE_MEMORY_BLOCKS];
uint8_t usable_memory_blocks;

uint8_t first_alloc_block;

uint32_t bitmap_size;
uint8_t* bitmap;

uint32_t memory_allocated, allocatable_memory;

atomic_flag pfa_spinlock = ATOMIC_FLAG_INIT;

// #define LOG_MEMORY

#ifdef LOG_MEMORY
#define LOG_MEM_ALLOCATED() { uint32_t percentage = 100000 * memory_allocated / allocatable_memory; LOG(TRACE, "Used memory : %u / %u bytes (%u.%u%u%u%%)", memory_allocated, allocatable_memory, percentage / 1000, (percentage / 100) % 10, (percentage / 10) % 10, percentage % 10); }
#else
#define LOG_MEM_ALLOCATED()
#endif

void pfa_detect_usable_memory();
void pfa_bitmap_init();
physical_address_t pfa_allocate_physical_page();
void pfa_free_physical_page(physical_address_t address);