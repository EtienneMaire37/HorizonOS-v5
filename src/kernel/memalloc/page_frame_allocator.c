#pragma once

uint64_t usable_memory;

struct mem_block
{
    physical_address_t address;
    uint32_t length;
};

struct mem_block usable_memory_map[64];
uint8_t usable_memory_blocks;

uint8_t first_alloc_block;
virtual_address_t first_alloc_page;

void pfa_detect_usable_memory()
{
    usable_memory = usable_memory_blocks = 0;

    LOG(INFO, "Usable memory map:");

    for (uint32_t i = 0; i < multiboot_info->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + i);
        physical_address_t addr = ((physical_address_t)mmmt->addr_high << 32) | mmmt->addr_low;
        uint32_t len = mmmt->len_low;
        if (mmmt->len_high)
            continue; // Ignore memory blocks above 4GB
        if (addr & 0xfff) // Align to page boundaries
        {
            uint32_t offset = 0x1000 - (addr & 0xfff);
            addr += offset;
            if (len < offset)
                continue;
            len -= offset;
        }
        len /= 0x1000;
        len *= 0x1000;
        if (len == 0)
            continue;
        if (addr + len <= virtual_address_to_physical((virtual_address_t)&_kernel_end))
            continue;
        while (addr < virtual_address_to_physical((virtual_address_t)&_kernel_end))
        {
            if (len < 0x1000)
                continue;
            len -= 0x1000;
            addr += 0x1000;
        }
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) 
        {
            if (usable_memory_blocks >= 64)
            {
                LOG(WARNING, "Too many memory blocks detected");
                break;
            }

            LOG(INFO, "   Memory block : address : 0x%lx ; length : %u", addr, len);
            usable_memory += len;

            usable_memory_map[usable_memory_blocks].address = addr;
            usable_memory_map[usable_memory_blocks].length = len;
            usable_memory_blocks++;
        }   
    }

    if (usable_memory == 0)
    {
        LOG(CRITICAL, "No usable memory detected");
        kabort();
    }

    LOG(INFO, "Detected %u bytes of usable memory", usable_memory); 
}

void pfa_bitmap_init()
{
    uint32_t bitmap_size = usable_memory / 0x1000 / 8;
    // if (usable_memory % 0x1000)
    //     bitmap_size++;
    LOG(INFO, "Allocating %u bytes of bitmap", bitmap_size);
    virtual_address_t address = physical_address_to_virtual(usable_memory_map[0].address);
    uint8_t* bitmap_start = (uint8_t*)address;
    uint8_t current_block = 0;
    for (uint32_t i = 0; i < bitmap_size; i++)
    {
        *(uint8_t*)address = 0;
        address++;
        if (address >= physical_address_to_virtual(usable_memory_map[current_block].address) + usable_memory_map[current_block].length)
        {
            current_block++;
            if (current_block >= usable_memory_blocks)
            {
                LOG(CRITICAL, "Not enough memory blocks to allocate bitmap");
                kabort();
            }
            address = usable_memory_map[current_block].address;
        }
        // LOG(DEBUG, "Bitmap address : 0x%x", address);
    }

    first_alloc_block = current_block;
    first_alloc_page = ((address + 0xfff) / 0x1000) * 0x1000;

    LOG(INFO, "Initialized page frame allocator bitmap at address 0x%x", bitmap_start);
    LOG(INFO, "First allocatable block : %u", first_alloc_block);
    LOG(INFO, "First allocatable page address : 0x%x", first_alloc_page);
}