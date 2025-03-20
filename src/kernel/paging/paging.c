#pragma once

uint8_t* get_physical_address_ptr(physical_address_t address)
{
    uint32_t addr = (uint32_t)address;
    uint32_t page = addr >> 12;
    uint16_t offset = addr & 0xfff;
    // if (page != current_phys_mem_page)   // ! TODO: Fix this (it doesn't work if you switch cr3 after a call)
    {
        struct page_directory_entry_4kb* pde = (struct page_directory_entry_4kb*)physical_address_to_virtual(current_cr3 + 4 * 767);
        if (!pde->present)
        {
            LOG(ERROR, "Page table 767 not present");
            abort();
            return NULL;
        }
        struct page_table_entry* pte = (struct page_table_entry*)physical_address_to_virtual((pde->address << 12) + 4 * 1021);
        if (!pte->present)
        {
            LOG(ERROR, "Page 767:1021 not present");
            abort();
            return NULL;
        }
        pte->address = page;
        current_phys_mem_page = page;
    }

    return (uint8_t*)(PHYS_MEM_PAGE_BOTTOM + offset);
}

uint8_t read_physical_address(physical_address_t address)
{
    if (address >> 32)
    {
        LOG(WARNING, "Tried to read from an adress over the 4GB limit (0x%lx)", address);
        return 0xff;
    }

    uint8_t* ptr = get_physical_address_ptr(address);

    if (ptr == NULL)
        return 0xff;
    
    return *ptr;
}

void write_physical_address(physical_address_t address, uint8_t value)
{
    if (address >> 32)
    {
        LOG(WARNING, "Tried to read from an adress over the 4GB limit (0x%lx)", address);
        return;
    }

    uint8_t* ptr = get_physical_address_ptr(address);

    if (ptr == NULL)
        return;
    
    *ptr = value;
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
    pd[index].cache_disable = 1;
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
    pt[index].cache_disable = 1;
    pt[index].write_through = 0;
    pt[index].dirty = 0;
    pt[index].address = (address >> 12);
    pt[index].page_attribute_table = 0;
    pt[index].user_supervisor = user_supervisor;
    pt[index].read_write = read_write;
    pt[index].present = 1;
} 

void set_page_by_address(struct page_directory_entry_4kb* pd, virtual_address_t vaddress, physical_address_t paddress, uint8_t user_supervisor, uint8_t read_write)
{
    LOG(CRITICAL, "Tried to use the set_page_by_address function");
    abort();
}

void remove_page_by_address(struct page_directory_entry_4kb* pd, virtual_address_t vaddress)
{
    struct virtual_address_layout* layout = (struct virtual_address_layout*)&vaddress;
    remove_page((struct page_table_entry*)(pd[layout->page_directory_entry].address << 12), layout->page_table_entry);
}