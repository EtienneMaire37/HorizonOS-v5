#pragma once

#include "../cpu/util.h"

struct mem_block
{
    physical_address_t address;
    size_t total_pages;
    size_t used_pages;
} __attribute__((packed));

#define MAX_USABLE_MEMORY_BLOCKS 64
