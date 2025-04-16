#pragma once

void set_current_phys_mem_page(uint32_t page)
{
    if (page == current_phys_mem_page) return;

    uint32_t* recursive_paging_pte = (uint32_t*)(((uint32_t)4 * 1024 * 1024 * 1023) | (4 * (767 * 1024 + 1021)));
    // *recursive_paging_pte = (page << 12) | ((*recursive_paging_pte) & 0xfff);
    *recursive_paging_pte = (page << 12) | 0b11;

    reload_page_directory();
    // asm volatile("invlpg [(767 * 1024 * 1024 + 1021 * 1024) * 4096]");   // !! Doesn't work on i386

    current_phys_mem_page = page;
}

uint8_t* get_physical_address_ptr(physical_address_t address)
{
    uint32_t addr = (uint32_t)address;
    uint32_t page = addr >> 12;
    uint16_t offset = addr & 0xfff;
    
    set_current_phys_mem_page(page);

    return (uint8_t*)(PHYS_MEM_PAGE_BOTTOM + offset);
}

uint8_t read_physical_address_1b(physical_address_t address)
{
    if (address >> 32)
    {
        LOG(WARNING, "Tried to read from an address over the 4GB limit (0x%lx)", address);
        return 0xff;
    }

    uint8_t* ptr = get_physical_address_ptr(address);

    if (ptr == NULL)
        return 0xff;
    
    return *ptr;
}

void write_physical_address_1b(physical_address_t address, uint8_t value)
{
    if (address >> 32)
    {
        LOG(WARNING, "Tried to write to an address over the 4GB limit (0x%lx)", address);
        return;
    }

    uint8_t* ptr = get_physical_address_ptr(address);

    if (ptr == NULL)
        return;
    
    *ptr = value;
}

uint16_t read_physical_address_2b(physical_address_t address)
{
    return read_physical_address_1b(address) | ((uint16_t)read_physical_address_1b(address + 1) << 8);
}

uint32_t read_physical_address_4b(physical_address_t address)
{
    return read_physical_address_1b(address) | ((uint32_t)read_physical_address_1b(address + 1) << 8) | ((uint16_t)read_physical_address_1b(address + 2) << 16) | ((uint16_t)read_physical_address_1b(address + 3) << 24);
}

void write_physical_address_2b(physical_address_t address, uint16_t value)
{
    write_physical_address_1b(address, value);
    write_physical_address_1b(address + 1, value >> 8);
}

void write_physical_address_4b(physical_address_t address, uint32_t value)
{
    write_physical_address_1b(address, value);
    write_physical_address_1b(address + 1, value >> 8);
    write_physical_address_1b(address + 2, value >> 16);
    write_physical_address_1b(address + 3, value >> 24);
}

void init_page_directory(struct page_directory_entry_4kb* pd)
{
    for(uint16_t i = 0; i < 1024; i++)
        pd[i].present = 0;
}

void init_page_table(struct page_table_entry* pt)
{
    for(uint16_t i = 0; i < 1024; i++)
        pt[i].present = 0;
}

void add_page_table(struct page_directory_entry_4kb* pd, uint16_t index, physical_address_t pt_address, uint8_t user_supervisor, uint8_t read_write)
{
    if (pt_address & 0xfff)
    {
        LOG(CRITICAL, "Tried to set a non page aligned page table");
        abort();
    }

    pd[index].page_size = 0;
    pd[index].cache_disable = 0;
    pd[index].write_through = 0;
    pd[index].address = (((uint32_t)pt_address) >> 12);
    pd[index].user_supervisor = user_supervisor;
    pd[index].read_write = read_write;
    pd[index].present = 1;
} 

void remove_page_table(struct page_directory_entry_4kb* pd, uint16_t index)
{
    pd[index].page_size = 0;
    pd[index].present = 0;
} 

void remove_page(struct page_table_entry* pt, uint16_t index)
{
    pt[index].present = 0;
} 

void set_page(struct page_table_entry* pt, uint16_t index, physical_address_t address, uint8_t user_supervisor, uint8_t read_write)
{
    if (address & 0xfff)
    {
        LOG(CRITICAL, "Tried to set a non page aligned page");
        abort();
    }

    pt[index].global = 0;
    pt[index].cache_disable = 0;
    pt[index].write_through = 0;
    pt[index].dirty = 0;
    pt[index].address = (address >> 12);
    pt[index].page_attribute_table = 0;
    pt[index].user_supervisor = user_supervisor;
    pt[index].read_write = read_write;
    pt[index].present = 1;
} 

void physical_init_page_directory(physical_address_t pd)
{
    for(uint16_t i = 0; i < 1024; i++)
        write_physical_address_4b(pd + 4 * i, 0);  // Not present
}

void physical_init_page_table(physical_address_t pt)
{
    for(uint16_t i = 0; i < 1024; i++)
        write_physical_address_4b(pt + 4 * i, 0);  // Not present
}

void physical_add_page_table(physical_address_t pd, uint16_t index, physical_address_t pt_address, uint8_t user_supervisor, uint8_t read_write)
{
    if (pt_address & 0xfff)
    {
        LOG(CRITICAL, "Tried to set a non page aligned page table");
        abort();
    }

    user_supervisor = user_supervisor != 0;
    read_write = read_write != 0;
    
    write_physical_address_4b(pd + 4 * index, 1 | (read_write << 1) | (user_supervisor << 2) | pt_address);
}

void physical_remove_page_table(physical_address_t pd, uint16_t index)
{
    write_physical_address_4b(pd + 4 * index, 0);
}

void physical_remove_page(physical_address_t pt, uint16_t index)
{
    write_physical_address_4b(pt + 4 * index, 0);
}

void physical_set_page(physical_address_t pt, uint16_t index, physical_address_t address, uint8_t user_supervisor, uint8_t read_write)
{
    if (address & 0xfff)
    {
        LOG(CRITICAL, "Tried to set a non page aligned page");
        abort();
    }

    user_supervisor = user_supervisor != 0;
    read_write = read_write != 0;
    
    write_physical_address_4b(pt + 4 * index, 1 | (read_write << 1) | (user_supervisor << 2) | address);
}