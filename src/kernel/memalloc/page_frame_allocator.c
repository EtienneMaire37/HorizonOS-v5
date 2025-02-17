#pragma once

uint64_t usable_memory;

void pfa_detect_usable_memory()
{
    usable_memory = 0;

    LOG(INFO, "Usable memory map:");

    for (uint32_t i = 0; i < multiboot_info->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + i);
        physical_address_t addr = ((physical_address_t)mmmt->addr_high << 32) | mmmt->addr_low;
        uint32_t len = mmmt->len_low;
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
            LOG(INFO, "   Memory block : address : 0x%lx ; length : %u", addr, len);
            usable_memory += len;
        }   
    }

    LOG(INFO, "Detected %u bytes of usable memory", usable_memory); 
}

