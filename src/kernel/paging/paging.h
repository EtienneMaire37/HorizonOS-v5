#pragma once

#define PG_SUPERVISOR   0
#define PG_USER         1

#define PG_READ_ONLY    0
#define PG_READ_WRITE   1

uint8_t physical_address_width = 0; // M

inline uint64_t get_physical_address_mask()
{
    if (physical_address_width == 0)
        abort();
    return ((uint64_t)1 << physical_address_width) - 1;
}