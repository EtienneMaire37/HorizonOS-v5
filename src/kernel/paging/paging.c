#pragma once

#include "../cpu/memory.h"

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
    
    write_physical_address_4b(pd + 4 * index, 0b0001 | (read_write << 1) | (user_supervisor << 2) | pt_address);  // Present x Write Back
}

void physical_remove_page_table(physical_address_t pd, uint16_t index)
{
    write_physical_address_4b(pd + 4 * index, 0b1000);
}

void physical_remove_page(physical_address_t pt, uint16_t index)
{
    write_physical_address_4b(pt + 4 * index, 0b1000);
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
    
    write_physical_address_4b(pt + 4 * index, 0b0001 | (read_write << 1) | (user_supervisor << 2) | address);   // Present x Write Back
}