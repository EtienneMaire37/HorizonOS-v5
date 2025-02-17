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

virtual_address_t pfa_allocate_page()
{
    uint8_t* bitmap_start = (uint8_t*)physical_address_to_virtual(usable_memory_map[0].address);
    uint32_t bitmap_size = usable_memory / 0x1000 / 8;

    for (uint32_t byte_idx = 0; byte_idx < bitmap_size; byte_idx++) 
    {
        uint8_t byte = bitmap_start[byte_idx];
        if (byte == 0xff) 
            continue;
        
        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) 
        {
            if (!(byte & (1 << bit_idx))) 
            {
                bitmap_start[byte_idx] |= (1 << bit_idx);
                
                uint32_t page_number = byte_idx * 8 + bit_idx;
                uint32_t remaining = page_number;
                
                for (uint8_t block_idx = 0; block_idx < usable_memory_blocks; block_idx++) 
                {
                    struct mem_block* block = &usable_memory_map[block_idx];
                    uint32_t pages_in_block = block->length / 0x1000;
                    
                    if (remaining < pages_in_block) 
                    {
                        physical_address_t phys_addr = block->address + (remaining * 0x1000);
                        virtual_address_t virt_addr = physical_address_to_virtual(phys_addr);
                        LOG(DEBUG, "Allocated page at 0x%x", virt_addr);
                        return virt_addr;
                    }
                    remaining -= pages_in_block;
                }

                LOG(CRITICAL, "Allocated page not in memory map!");
                kabort();
            }
        }
    }
    
    LOG(CRITICAL, "Out of memory!");
    kabort();
    return 0;
}

void pfa_free_page(virtual_address_t address)
{
    physical_address_t phys_addr = virtual_address_to_physical(address);
    uint32_t page_number = 0;
    
    for (uint8_t block_idx = 0; block_idx < usable_memory_blocks; block_idx++) 
    {
        struct mem_block* block = &usable_memory_map[block_idx];
        
        if (phys_addr >= block->address && 
            phys_addr < (block->address + block->length)) 
        {
            page_number += (phys_addr - block->address) / 0x1000;
            break;
        }

        page_number += block->length / 0x1000;
    }

    uint32_t byte_idx = page_number / 8;
    uint8_t bit_idx = page_number % 8;
    uint8_t* bitmap = (uint8_t*)physical_address_to_virtual(usable_memory_map[0].address);
    
    bitmap[byte_idx] &= ~(1 << bit_idx);

    LOG(DEBUG, "Freed page at 0x%x", address);
}