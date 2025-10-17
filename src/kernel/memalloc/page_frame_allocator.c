#pragma once

void pfa_detect_usable_memory() 
{
    usable_memory = usable_memory_blocks = 0;
    bitmap = NULL;
    memory_allocated = 0;
    allocatable_memory = 0;

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

        uint32_t align = addr & 0xfff;
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

    first_alloc_block = 0;
    uint32_t total_pages = 0;
    for (uint32_t i = 0; i < usable_memory_blocks; i++)
        total_pages += usable_memory_map[i].length / 0x1000;
    
    bitmap_size = (total_pages + 7) / 8;
    uint32_t bitmap_pages = (bitmap_size + 0xfff) / 0x1000;

    while (first_alloc_block < usable_memory_blocks && usable_memory_map[first_alloc_block].length < bitmap_pages)
        first_alloc_block++;

    bitmap = (uint8_t*)physical_address_to_virtual(usable_memory_map[first_alloc_block].address);

    if (usable_memory == 0 || first_alloc_block >= usable_memory_blocks) 
        goto no_memory;

    usable_memory_map[first_alloc_block].length -= 0x1000 * bitmap_pages;
    usable_memory_map[first_alloc_block].address += 0x1000 * bitmap_pages;
    if (usable_memory_map[first_alloc_block].length == 0)
        first_alloc_block++;

    if (first_alloc_block >= usable_memory_blocks)
        goto no_memory;

    memset(bitmap, 0, bitmap_size);

    for (uint8_t i = first_alloc_block; i < usable_memory_blocks; i++)
        allocatable_memory += usable_memory_map[i].length;

    LOG(INFO, "Detected %u bytes of allocatable memory", allocatable_memory);
    return;

no_memory:
    LOG(CRITICAL, "No usable memory detected");
    abort();
}

physical_address_t pfa_allocate_physical_page() 
{
    if (memory_allocated + 0x1000 > allocatable_memory) 
    {
        LOG(CRITICAL, "Out of memory!");
        return physical_null;
    }

    acquire_spinlock(&pfa_spinlock);

    for (uint32_t i = 0; i < bitmap_size; i++) 
    {
        uint8_t byte = bitmap[i];
        if (byte == 0xff) continue;

        for (uint8_t bit = 0; bit < 8; bit++) 
        {
            if (!(byte & (1 << bit))) 
            {
                bitmap[i] |= (1 << bit);
                memory_allocated += 0x1000;

                uint32_t page_index = i * 8 + bit;

                uint32_t remaining = page_index;
                for (uint32_t j = first_alloc_block; j < usable_memory_blocks; j++) 
                {
                    uint32_t block_pages = usable_memory_map[j].length / 0x1000;
                    if (remaining < block_pages) 
                    {
                        physical_address_t addr = usable_memory_map[j].address + remaining * 0x1000;
                        LOG_MEM_ALLOCATED();
                        release_spinlock(&pfa_spinlock);
                        return addr;
                    }
                    remaining -= block_pages;
                }

                LOG(CRITICAL, "Invalid page index %u (%u / %u bytes used)", page_index, memory_allocated, allocatable_memory);
                abort();
                return physical_null;
            }
        }
    }

    LOG(CRITICAL, "Out of memory!");
    release_spinlock(&pfa_spinlock);
    return physical_null;
}

void pfa_free_physical_page(physical_address_t address) 
{
    if (address == physical_null) 
    {
        LOG(WARNING, "Kernel tried to free NULL");
        return;
    }

    if (address & 0xfff) 
    {
        LOG(CRITICAL, "Unaligned address (0x%lx)", address);
        abort();
    }

    uint32_t page_index = 0;
    for (uint32_t i = first_alloc_block; i < usable_memory_blocks; i++) 
    {
        if (address >= usable_memory_map[i].address && 
            address < usable_memory_map[i].address + usable_memory_map[i].length) 
        {
            page_index += (address - usable_memory_map[i].address) / 0x1000;
            break;
        }
        page_index += usable_memory_map[i].length / 0x1000;
    }

    uint32_t byte = page_index / 8;
    uint8_t bit = page_index & 0b111;
    if (byte >= bitmap_size) return;
    acquire_spinlock(&pfa_spinlock);
    bitmap[byte] &= ~(1 << bit);

    memory_allocated -= 0x1000;
    release_spinlock(&pfa_spinlock);
    LOG_MEM_ALLOCATED();
}