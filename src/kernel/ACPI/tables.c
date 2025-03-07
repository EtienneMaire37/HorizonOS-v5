#pragma once

void bios_get_ebda_pointer()
{
    uint32_t ebda_address = (*(uint16_t*)0x40e) << 4;
    LOG(INFO, "EBDA address : 0x%x", ebda_address);
    ebda = (uint8_t*)ebda_address;
}

bool acpi_table_valid(struct sdt_header* header)
{
    uint8_t sum = 0;
    for (uint32_t i = 0; i < header->length; i++)
        sum += ((uint8_t*)&header[0])[i];
    return sum == 0;
}

bool acpi_rsdp_valid(struct rsdp_table* table)
{
    uint8_t sum = 0;
    for (uint32_t i = 0; i < (acpi_10 ? 20 : table->length); i++)
        sum += ((uint8_t*)&table[0])[i];
    return sum == 0;
}

void acpi_find_tables()
{
    rsdp = rsdt = NULL;
    LOG(INFO, "Searching for the RSDP");
    printf("Searching for the RSDP\n");

    LOG(DEBUG, "\tChecking the 1rst KB of EBDA");

    // Check EBDA
    for (uint8_t i = 0; i < 64; i++)
    {
        virtual_address_t address = (uint32_t)ebda + 16 * (uint32_t)i;
        if (memcmp((void*)address, "RSD PTR ", 8) == 0)
        {
            rsdp = (struct rsdp_table*)address;
            goto found_rsdp;
        }
    }
    LOG(DEBUG, "\tChecking the 0xe0000 - 0xfffff");

    // Check 0xe0000 - 0xfffff
    for (uint16_t i = 0; i < 0x2000; i++)
    {
        virtual_address_t address = (uint32_t)0xe0000 + 16 * (uint32_t)i;
        if (memcmp((void*)address, "RSD PTR ", 8) == 0)
        {
            rsdp = (struct rsdp_table*)address;
            goto found_rsdp;
        }
    }

    LOG(CRITICAL, "Couldn't find the RSDP table");
    printf("Couldn't find the RSDP table\n");
    abort();

found_rsdp:
    LOG(INFO, "Found rsdp at address 0x%x", rsdp);

    acpi_10 = rsdp->revision == 0;
    
    LOG(INFO, "ACPI version : %s", acpi_10 ? "1.0" : "2.0+");
    printf("ACPI version : %s\n", acpi_10 ? "1.0" : "2.0+");

    bool table_valid = acpi_rsdp_valid((void*)rsdp);

    if (!table_valid)
    {
        LOG(CRITICAL, "Invalid RSDP table");
        printf("Invalid RSDP table\n");
        abort();
    }

    // if (acpi_10)
    // {
    //     rsdt = (struct rsdt_table*)rsdp->rsdt_address;
    //     xsdt = NULL;

    //     LOG(INFO, "RSDT address : 0x%x", rsdt);

    //     sdt_count = (rsdt->header.length - sizeof(struct sdt_header)) / 4;
    // }
    // else
    // {
    //     uint64_t address = rsdp->xsdt_address;
    //     if (address >> 32)
    //     {
    //         LOG(CRITICAL, "64 bit XSDT address");
    //         abort();
    //     }

    //     xsdt = (struct xsdt_table*)((uint32_t)address);
    //     rsdt = NULL;

    //     LOG(INFO, "XSDT address : 0x%x", xsdt);

    //     sdt_count = (xsdt->header.length - sizeof(struct sdt_header)) / 8;
    // }

    // LOG(INFO, "%u SDT tables detected", sdt_count);
}

uint32_t read_sdt_ptr(uint32_t index)
{
    if (index >= sdt_count)
    {
        LOG(CRITICAL, "Kernel tried to read an invalid SDT (%u / %u)", index, sdt_count);
        abort();
    }

    if (acpi_10)    // ACPI 1.0
    {
        uint32_t* sdt_ptrs = &(((uint8_t*)rsdt)[sizeof(struct sdt_header)]);
        return sdt_ptrs[index];
    }
    else            // ACPI 2.0+
    {
        uint64_t* sdt_ptrs = &(((uint8_t*)rsdt)[sizeof(struct sdt_header)]);
        uint64_t address = sdt_ptrs[index];
        if (address >> 32)
        {
            LOG(CRITICAL, "64 bit SDT address at index %u", index);
            abort();
        }
        return (uint32_t)address;
    }
}