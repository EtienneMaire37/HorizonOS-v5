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
virtual_address_t first_alloc_page;
uint32_t bitmap_size;

uint32_t memory_allocated, allocatable_memory;

uint8_t* pma_page_address;

#define LOG_MEM_ALLOCATED() // { float percentage = (float)memory_allocated / (float)allocatable_memory * 100; LOG(TRACE, "Used memory : %u / %u bytes (%u.%u%u%u%%)", memory_allocated, allocatable_memory, (int)percentage, (int)(percentage * 10) % 10, (int)(percentage * 100) % 10, (int)(percentage * 1000) % 10); }

void pfa_detect_usable_memory();
void pfa_bitmap_init();
virtual_address_t pfa_allocate_page();
void pfa_free_page(virtual_address_t address);