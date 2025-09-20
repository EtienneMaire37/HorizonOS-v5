#pragma once

#define PHYS_MEM_PAGE_TOP      TASK_KERNEL_STACK_BOTTOM_ADDRESS
#define PHYS_MEM_PAGE_BOTTOM   (PHYS_MEM_PAGE_TOP - 0x1000)
uint32_t current_phys_mem_page;

const int physical_memory_page_index = (PHYS_MEM_PAGE_BOTTOM - (uint32_t)767 * 0x400000) / 0x1000;

void invlpg(uint32_t addr)
{
    asm volatile("invlpg [%0]" :: "r" (addr) : "memory");
}

void load_pd_by_physaddr(physical_address_t addr)
{
    if (addr >> 32)
    {
        LOG(CRITICAL, "Tried to load a page directory above 4GB");
        abort();
    }
    
    asm volatile("mov cr3, eax" : : "a" ((uint32_t)addr));
    
    current_phys_mem_page = 0xffffffff;
}

void load_pd(void* ptr)
{
    load_pd_by_physaddr(virtual_address_to_physical((virtual_address_t)ptr));
}

void set_current_phys_mem_page(uint32_t page)
{
#define DONT_RELOAD_PHYS
#ifdef DONT_RELOAD_PHYS
    if (page == current_phys_mem_page) return;
#endif

    uint32_t* recursive_paging_pte = (uint32_t*)(((uint32_t)4 * 1024 * 1024 * 1023) | (4 * (767 * 1024 + 1021)));
    // *recursive_paging_pte = (page << 12) | ((*recursive_paging_pte) & 0xfff);
    *recursive_paging_pte = (page << 12) | 0b1011; // * Write-through caching | Read write | Present

    // reload_page_directory();

    // !! Only works on i486+
    invlpg((uint32_t)recursive_paging_pte);
    invlpg(4096 * (uint32_t)(1024 * 767 + 1021));

    current_phys_mem_page = page;
}

uint8_t* get_physical_address_ptr(physical_address_t address)
{
    if (address >> 32) return NULL;

    uint32_t addr = (uint32_t)address;
    if (addr < 0x100000) return (uint8_t*)addr;
    if (addr < (uint32_t)4 * 1024 * 1024 * 1023 - 0xc0000000) return (uint8_t*)(addr + 0xc0000000);
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

    volatile uint8_t* ptr = get_physical_address_ptr(address);

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

    volatile uint8_t* ptr = get_physical_address_ptr(address);

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

static uint8_t page_tmp[4096] = {0};

void copy_page(physical_address_t from, physical_address_t to)
{
    if ((from & 0xfff) || (to & 0xfff))
    {
        LOG(ERROR, "Tried to copy non page aligned pages : 0x%lx to 0x%lx", from, to);
        abort();
    }

    for (uint16_t i = 0; i < 4096; i++)
        page_tmp[i] = read_physical_address_1b(from + i);
    for (uint16_t i = 0; i < 4096; i++)
        write_physical_address_1b(to + i, page_tmp[i]);
}

void memset_page(physical_address_t page, uint8_t value)
{
    if (page & 0xfff)
    {
        LOG(ERROR, "Tried to memset a non page aligned page : 0x%lx", page);
        abort();
    }

    for (uint16_t i = 0; i < 4096; i++)
        write_physical_address_1b(page + i, value);
}