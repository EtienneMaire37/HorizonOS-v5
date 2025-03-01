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
    
    LOG(DEBUG, "\tChecking the 1rst KB of EBDA");

    // Check EBDA
    for (uint8_t i = 0; i < 64; i++)
    {
        virtual_address_t address = (uint32_t)ebda + 16 * (uint32_t)i;
        if (kmemcmp((void*)address, "RSD PTR ", 8) == 0)
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
        if (kmemcmp((void*)address, "RSD PTR ", 8) == 0)
        {
            rsdp = (struct rsdp_table*)address;
            goto found_rsdp;
        }
    }

    LOG(CRITICAL, "Couldn't find the RSDP table");
    kabort();

found_rsdp:
    LOG(INFO, "Found rsdp at address 0x%x", rsdp);

    acpi_10 = rsdp->revision == 0;
    
    LOG(INFO, "ACPI version : %s", acpi_10 ? "1.0" : "2.0+");

    bool table_valid = acpi_rsdp_valid((void*)rsdp);

    if (!table_valid)
    {
        LOG(CRITICAL, "Invalid RSDP table");
        kabort();
    }
}