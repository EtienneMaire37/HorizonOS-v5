#pragma once

void pfa_detect_usable_memory()
{
    usable_memory = usable_memory_blocks = 0;

    LOG(INFO, "Usable memory map:");

    putchar('\n');

    for (uint32_t i = 0; i < multiboot_info->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + i);
        physical_address_t addr = ((physical_address_t)mmmt->addr_high << 32) | mmmt->addr_low;
        uint64_t len = ((physical_address_t)mmmt->len_high << 32) | mmmt->len_low;
        if (addr >= 0xffffffff)
            continue;
        else if (addr + len > 0xffffffff)
            len = 0xffffffff - addr; // -(uint32_t)addr
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
        if (addr + len <= virtual_address_to_physical(kernel_end))
            continue;
        while (addr < virtual_address_to_physical(kernel_end))
        {
            if (len < 0x1000)
                continue;
            len -= 0x1000;
            addr += 0x1000;
        }
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) 
        {
            if (usable_memory_blocks >= MAX_USABLE_MEMORY_BLOCKS)
            {
                LOG(WARNING, "Too many memory blocks detected");
                break;
            }

            LOG(INFO, "   Memory block : address : 0x%lx ; length : %lu", addr, len);
            usable_memory += len;

            uint32_t len32 = len;

            usable_memory_map[usable_memory_blocks].address = addr;
            usable_memory_map[usable_memory_blocks].length = len32;
            // printf("Memory block %u : address : 0x%lx ; length : %u\n", usable_memory_blocks, usable_memory_map[usable_memory_blocks].address, usable_memory_map[usable_memory_blocks].length);
            usable_memory_blocks++;
        }   
    }

    if (usable_memory == 0)
    {
        LOG(CRITICAL, "No usable memory detected");
        abort();
    }

    LOG(INFO, "Detected %u bytes of usable memory", usable_memory); 
}

void pfa_bitmap_init()
{
    // Optimal bitmap size calculation
    // Let X be the usable memory
    // Let B be the bitmap size
    // then we find that (X - B) / (4096 * 8) = B
    // <=> B = X / (8 * 4096 + 1) = X / 32769
    
    bitmap_size = (uint32_t)(usable_memory + 32768) / 32769;   // Round up the byte
    memory_allocated = 0;
    allocatable_memory = usable_memory - bitmap_size;
    LOG(DEBUG, "Allocating %u bytes of bitmap", bitmap_size);
    virtual_address_t address = physical_address_to_virtual(usable_memory_map[0].address);
    uint8_t* bitmap_start = (uint8_t*)address;
    uint8_t current_block = 0;
    for (uint32_t i = 0; i < bitmap_size; i++)
    {
        *(uint8_t*)address = 0;
        address++;
        if (address >= (uint64_t)physical_address_to_virtual(usable_memory_map[current_block].address) + usable_memory_map[current_block].length)
        {
            // LOG(DEBUG, "Switching memory block from address: 0x%lx, length: %u", usable_memory_map[current_block].address, usable_memory_map[current_block].length);
            current_block++;
            if (current_block >= usable_memory_blocks)
            {
                LOG(CRITICAL, "Not enough memory blocks to allocate bitmap");
                abort();
            }
            address = usable_memory_map[current_block].address; // max(address, usable_memory_map[current_block].address);
        }
        // LOG(DEBUG, "Bitmap address : 0x%x", address);
    }

    first_alloc_block = current_block;
    first_alloc_page = ((address + 0xfff) / 0x1000) * 0x1000;

    LOG(DEBUG, "Initialized page frame allocator bitmap at address 0x%x", bitmap_start);
    LOG(DEBUG, "First allocatable block : %u", first_alloc_block);
    LOG(DEBUG, "First allocatable page address : 0x%x", first_alloc_page);

    pma_page_address = (uint8_t*)pfa_allocate_page();
}

physical_address_t pfa_allocate_physical_page()
{
    if (memory_allocated + 0x1000 > allocatable_memory)
    {
        LOG(CRITICAL, "Out of memory !");
        abort();
    }

    uint32_t bitmap_byte_address = physical_address_to_virtual(usable_memory_map[0].address);
    uint8_t current_block = 0;
    for (uint32_t i = 0; i < bitmap_size; i++)
    {
        uint8_t bb = *(uint8_t*)bitmap_byte_address;
        if (bb != 0xff)
        {
            for (uint8_t j = 0; j < 8; j++)
            {
                uint8_t bit = 1 << j;
                if (!(bb & bit))
                {
                    bb |= bit;
                    uint32_t page = i * 8 + j;
                    physical_address_t address = virtual_address_to_physical(first_alloc_page);
                    current_block = 0;
                    for (uint32_t k = 0; k < page; k++)
                    {
                        address += 0x1000;
                        if (address >= usable_memory_map[current_block].address + usable_memory_map[current_block].length)
                        {
                            current_block++;
                            if (current_block >= usable_memory_blocks)
                            {
                                LOG(CRITICAL, "Out of memory !");
                                abort();
                            }
                            address = usable_memory_map[current_block].address;
                        }
                    }
                    memory_allocated += 0x1000;
                    *(uint8_t*)bitmap_byte_address = bb;
                    LOG_MEM_ALLOCATED();
                    return address;
                }
            }
        }

        bitmap_byte_address++;
        if (virtual_address_to_physical(bitmap_byte_address) >= usable_memory_map[current_block].address + usable_memory_map[current_block].length)
        {
            current_block++;
            if (current_block >= usable_memory_blocks)
            {
                LOG(CRITICAL, "Out of memory !");
                abort();
            }
            bitmap_byte_address = physical_address_to_virtual(usable_memory_map[current_block].address);
        }
    }

    LOG(CRITICAL, "Out of memory !");
    abort();
    return 0;
}

virtual_address_t pfa_allocate_page()
{
    return physical_address_to_virtual(pfa_allocate_physical_page());
}

void pfa_free_physical_page(physical_address_t address)
{
    if (address & 0xfff)
    {
        LOG(CRITICAL, "Tried to free a non page aligned address");
        abort();
    }

    // return;

    physical_address_t current_address = virtual_address_to_physical(first_alloc_page);
    uint8_t current_block_bitmap = 0, current_block_addr_index = 0;
    uint32_t bitmap_byte_address = physical_address_to_virtual(usable_memory_map[0].address);
    while(true)
    {
        uint32_t offset = 0;
        if (current_block_addr_index >= usable_memory_blocks - 1)
            offset = 0xffffffff;
        else
            offset = physical_address_to_virtual(usable_memory_map[current_block_addr_index + 1].address - usable_memory_map[current_block_addr_index].address);

        physical_address_t next_addr = current_address + offset;

        if (address <= next_addr)
        {
            uint8_t* byte_ptr = (uint8_t*)(bitmap_byte_address + (uint32_t)((address - current_address) / (4096 * 8)));
            uint8_t bit = ((address - current_address) / 4096) % 8;
            *byte_ptr &= ~(1 << bit);
            
            memory_allocated -= 0x1000;
            LOG_MEM_ALLOCATED();
            return;
        }

        if (offset == 0xffffffff)
            return;

        bitmap_byte_address += offset / (4096 * 8);
        if (virtual_address_to_physical(bitmap_byte_address) >= usable_memory_map[current_block_bitmap].address + usable_memory_map[current_block_bitmap].length)
        {
            current_block_bitmap++;
            if (current_block_bitmap >= usable_memory_blocks)
            {
                LOG(CRITICAL, "Out of memory !");
                abort();
            }
            bitmap_byte_address = physical_address_to_virtual(usable_memory_map[current_block_bitmap].address);
        }

        current_block_addr_index++;
    }
}

void pfa_free_page(virtual_address_t address)
{
    pfa_free_physical_page(virtual_address_to_physical(address));
}