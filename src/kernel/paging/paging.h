#pragma once

uint8_t physical_address_width = 0; // M

inline uint64_t get_physical_address_mask()
{
    if (physical_address_width == 0)
        abort();
    return (1 << physical_address_width) - 1;
}