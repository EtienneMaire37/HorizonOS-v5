#pragma once

void pfa_detect_usable_memory()
{
    usable_memory = usable_memory_blocks = 0;

    LOG(INFO, "Usable memory map:");

    kputchar('\n');

    for (uint32_t i = 0; i < multiboot_info->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + i);
        physical_address_t addr = ((physical_address_t)mmmt->addr_high << 32) | mmmt->addr_low;
        uint64_t len = ((physical_address_t)mmmt->len_high << 32) | mmmt->len_low;
        // if (addr + len > 0xffffffff)
        //     continue; // Ignore memory blocks above 4GB
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
            // kprintf("Memory block %u : address : 0x%lx ; length : %u\n", usable_memory_blocks, usable_memory_map[usable_memory_blocks].address, usable_memory_map[usable_memory_blocks].length);
            usable_memory_blocks++;

            // kprintf("len : %u\n", len32);
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
    // Optimal bitmap size calculation
    // Let X be the usable memory
    // Let (un) = X
    // Let f(x) = (X - x) / 0x1000 / 8 so that un+1=f(un)
    // Using the fixed point theorem, we can find that x = 32768/32769 * X
    
    bitmap_size = (uint32_t)(usable_memory / 0x1000 / 8 * 32768 + 32768) / 32769;   // Round up the byte
    memory_allocated = 0;
    allocatable_memory = usable_memory - bitmap_size;
    // if (usable_memory % 0x1000)
    //     bitmap_size++;
    LOG(DEBUG, "Allocating %u bytes of bitmap", bitmap_size);
    virtual_address_t address = physical_address_to_virtual(usable_memory_map[0].address);
    uint8_t* bitmap_start = (uint8_t*)address;
    uint8_t current_block = 0;
    for (uint32_t i = 0; i < bitmap_size; i++)
    {
        // LOG(DEBUG, "Bitmap address : 0x%x", address);
        *(uint8_t*)address = 0;
        address++;
        if (address >= (uint64_t)physical_address_to_virtual(usable_memory_map[current_block].address) + usable_memory_map[current_block].length)
        {
            // LOG(DEBUG, "Switching memory block from address: 0x%lx, length: %u", usable_memory_map[current_block].address, usable_memory_map[current_block].length);
            current_block++;
            if (current_block >= usable_memory_blocks)
            {
                LOG(CRITICAL, "Not enough memory blocks to allocate bitmap");
                kabort();
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
}

physical_address_t pfa_allocate_physical_page()
{
    if (memory_allocated + 0x1000 > allocatable_memory)
    {
        LOG(CRITICAL, "Out of memory !");
        kabort();
    }

    uint32_t bitmap_byte_address = physical_address_to_virtual(usable_memory_map[0].address);
    uint8_t current_block = 0;
    for (uint32_t i = 0; i < bitmap_size; i++)
    {
        if (*(uint8_t*)bitmap_byte_address != 0xff)
        {
            for (uint8_t j = 0; j < 8; j++)
            {
                uint8_t bit = 1 << j;
                if (!((*(uint8_t*)bitmap_byte_address) & bit))
                {
                    *(uint8_t*)bitmap_byte_address |= bit;
                    uint32_t page = i * 8 + j;
                    // virtual_address_t address = page * 4096 + first_alloc_page;
                    // virtual_address_t address = first_alloc_page;
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
                                kabort();
                            }
                            address = usable_memory_map[current_block].address;
                        }
                    }
                    // LOG(DEBUG, "Allocated page at 0x%x", address);
                    memory_allocated += 0x1000;
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
                kabort();
            }
            bitmap_byte_address = physical_address_to_virtual(usable_memory_map[current_block].address);
        }

        if(bitmap_byte_address >= 0xffffffff)
        {
            LOG(CRITICAL, "Out of memory !");
            kabort();
        }
    }

    LOG(CRITICAL, "Out of memory !");
    kabort();
    return 0;
}

virtual_address_t pfa_allocate_page()
{
    return physical_address_to_virtual(pfa_allocate_physical_page());
}

void pfa_free_page(virtual_address_t address)
{
    if (address & 0xfff)
    {
        LOG(CRITICAL, "Tried to free a non page aligned address");
        kabort();
    }

    virtual_address_t current_address = first_alloc_page;
    uint8_t current_block = 0;
    virtual_address_t byte_address = physical_address_to_virtual(usable_memory_map[0].address);
    for (uint32_t i = 0; i < bitmap_size; i++)
    {
        for(uint8_t j = 0; j < 8; j++)
        {
            current_address += 0x1000;
            if (current_address == address)
            {
                *(uint8_t*)byte_address &= ~(1 << j);
                memory_allocated -= 0x1000;
                LOG_MEM_ALLOCATED();
                return;
            }
        }

        if (address < current_address)
        {
            LOG(CRITICAL, "Tried to free an unallocated page (address : 0x%x)", address);
            kabort();
        }

        byte_address++;
        if (virtual_address_to_physical(byte_address) >= usable_memory_map[current_block].address + usable_memory_map[current_block].length)
        {
            current_block++;
            if (current_block >= usable_memory_blocks)
            {
                LOG(CRITICAL, "Tried to free an unallocated page (address : 0x%x)", address);
                kabort();
            }
            byte_address = physical_address_to_virtual(usable_memory_map[current_block].address);
        }
    }
}