#pragma once

void pfa_detect_usable_memory() 
{
    usable_memory = usable_memory_blocks = 0;

    LOG(INFO, "Usable memory map:");

    physical_address_t kernel_end_phys = virtual_address_to_physical(kernel_end);

    for (uint32_t i = 0; i < multiboot_info->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + i);
        physical_address_t addr = ((physical_address_t)mmmt->addr_high << 32) | mmmt->addr_low;
        uint64_t len = ((physical_address_t)mmmt->len_high << 32) | mmmt->len_low;

        if (mmmt->type != MULTIBOOT_MEMORY_AVAILABLE || addr >= 0xffffffff)
            continue;
        if (addr + len > 0xffffffff)
            len = 0xffffffff - addr;

        if (addr < kernel_end_phys) 
        {
            if (addr + len <= kernel_end_phys)
                continue;
            len -= kernel_end_phys - addr;
            addr = kernel_end_phys;
        }

        uint32_t align = addr % 0x1000;
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
        usable_memory_map[usable_memory_blocks].length = (uint32_t)len;
        usable_memory += len;
        usable_memory_blocks++;

        LOG(INFO, "   Memory block : address : 0x%lx ; length : %lu", addr, len);
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
    uint32_t total_pages = 0;
    for (uint32_t i = 0; i < usable_memory_blocks; i++)
        total_pages += usable_memory_map[i].length / 0x1000;
    
    uint32_t bitmap_size_bytes = (total_pages + 7) / 8;
    uint32_t bitmap_size_pages = (bitmap_size_bytes + 0xfff) / 0x1000;
    bitmap_size = bitmap_size_pages * 0x1000;

    uint32_t chosen_block = 0xffffffff;
    for (uint32_t i = 0; i < usable_memory_blocks; i++) 
    {
        if (usable_memory_map[i].length >= bitmap_size) 
        {
            chosen_block = i;
            break;
        }
    }

    if (chosen_block == 0xffffffff) 
    {
        LOG(CRITICAL, "No block large enough for bitmap");
        abort();
    }

    physical_address_t bitmap_phys = usable_memory_map[chosen_block].address;
    usable_memory_map[chosen_block].address += bitmap_size;
    usable_memory_map[chosen_block].length -= bitmap_size;

    if (usable_memory_map[chosen_block].length == 0) 
    {
        for (uint32_t i = chosen_block; i < usable_memory_blocks - 1; i++)
            usable_memory_map[i] = usable_memory_map[i + 1];
        usable_memory_blocks--;
    }

    virtual_address_t bitmap_virt = physical_address_to_virtual(bitmap_phys);
    memset((void*)bitmap_virt, 0, bitmap_size);

    for (uint32_t i = 0; i < bitmap_size_pages; i++) 
    {
        physical_address_t page = bitmap_phys + i * 0x1000;
        uint32_t page_index = 0;

        for (uint32_t j = 0; j < usable_memory_blocks; j++) 
        {
            if (page >= usable_memory_map[j].address && 
                page < usable_memory_map[j].address + usable_memory_map[j].length) 
            {
                page_index += (page - usable_memory_map[j].address) / 0x1000;
                break;
            }
            page_index += usable_memory_map[j].length / 0x1000;
        }

        uint32_t byte_idx = page_index / 8;
        uint8_t bit_idx = page_index % 8;
        ((uint8_t*)bitmap_virt)[byte_idx] |= (1 << bit_idx);
    }

    usable_memory -= bitmap_size;
    allocatable_memory = usable_memory;
    memory_allocated = bitmap_size;

    LOG(DEBUG, "Bitmap initialized at 0x%x (size %u bytes)", bitmap_virt, bitmap_size);
}

physical_address_t pfa_allocate_physical_page() 
{
    if (memory_allocated + 0x1000 > allocatable_memory) 
    {
        LOG(CRITICAL, "Out of memory!");
        abort();
    }

    virtual_address_t bitmap_virt = physical_address_to_virtual(usable_memory_map[0].address);
    for (uint32_t i = 0; i < bitmap_size; i++) 
    {
        uint8_t byte = ((uint8_t*)bitmap_virt)[i];
        if (byte == 0xff) continue;

        for (uint8_t bit = 0; bit < 8; bit++) 
        {
            if (!(byte & (1 << bit))) 
            {
                ((uint8_t*)bitmap_virt)[i] |= (1 << bit);
                memory_allocated += 0x1000;

                uint32_t page_index = i * 8 + bit;

                uint32_t remaining = page_index;
                for (uint32_t j = 0; j < usable_memory_blocks; j++) 
                {
                    uint32_t block_pages = usable_memory_map[j].length / 0x1000;
                    if (remaining < block_pages) 
                    {
                        physical_address_t addr = usable_memory_map[j].address + remaining * 0x1000;
                        LOG_MEM_ALLOCATED();
                        // LOG(DEBUG, "0x%lx", addr);
                        return addr;
                    }
                    remaining -= block_pages;
                }

                LOG(CRITICAL, "Invalid page index");
                abort();
            }
        }
    }

    LOG(CRITICAL, "Out of memory!");
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
        LOG(CRITICAL, "Unaligned address");
        abort();
    }

    uint32_t page_index = 0;
    for (uint32_t i = 0; i < usable_memory_blocks; i++) 
    {
        if (address >= usable_memory_map[i].address && 
            address < usable_memory_map[i].address + usable_memory_map[i].length) 
        {
            page_index += (address - usable_memory_map[i].address) / 0x1000;
            break;
        }
        page_index += usable_memory_map[i].length / 0x1000;
    }

    uint32_t byte_idx = page_index / 8;
    uint8_t bit_idx = page_index % 8;
    ((uint8_t*)physical_address_to_virtual(usable_memory_map[0].address))[byte_idx] &= ~(1 << bit_idx);

    memory_allocated -= 0x1000;
    LOG_MEM_ALLOCATED();
}

void pfa_free_page(virtual_address_t address)
{
    pfa_free_physical_page(virtual_address_to_physical(address));
}