#pragma once

uint64_t usable_memory;

struct mem_block
{
    virtual_address_t address;
    uint32_t length;
};

struct mem_block usable_memory_map[64];
uint8_t usable_memory_blocks;

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
            LOG(INFO, "   Memory block : address : 0x%lx ; length : %u", addr, len);
            usable_memory += len;

            usable_memory_map[usable_memory_blocks].address = addr;
            usable_memory_map[usable_memory_blocks].length = len;
            usable_memory_blocks++;
        }   
    }

    LOG(INFO, "Detected %u bytes of usable memory", usable_memory); 
}
