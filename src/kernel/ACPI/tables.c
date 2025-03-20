#pragma once

uint32_t table_read_bytes(physical_address_t table_address, uint32_t offset, uint8_t size, bool little_endian)
{
    if (size > 4)
    {
        LOG(ERROR, "Kernel tried to read more than 4 bytes of a table");
        abort();
    }

    uint32_t result = 0;
    for (uint8_t i = 0; i < size; i++)
    {
        if (little_endian)
        {
            result |= ((uint32_t)read_physical_address(table_address + offset + i)) << (8 * i);
        }
        else
        {
            result <<= 8;
            result |= read_physical_address(table_address + offset + i);
        }
    }

    return result;
}

void bios_get_ebda_pointer()
{
    uint32_t ebda_address = (*(uint16_t*)0x40e) << 4;
    LOG(INFO, "EBDA address : 0x%x", ebda_address);
    ebda = (uint8_t*)ebda_address;
}

bool acpi_table_valid(physical_address_t table_address)
{
    uint8_t sum = 0;
    uint32_t length = table_read_member(struct sdt_header, table_address, length, true);
    // printf("len: %u\n", length);
    for (uint32_t i = 0; i < length; i++)
    {
        // printf("%u (%u)\n", i, table_read_member(struct sdt_header, table_address, length, true));
        sum += table_read_bytes(table_address, i, 1, true);
    }
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
    // rsdp = rsdt = NULL;
    rsdp = NULL;
    // rsdt = NULL;
    rsdt_address = xsdt_address = 0;
    fadt_address = 0;
    
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

    if (acpi_10)
    {
        rsdt_address = (physical_address_t)rsdp->rsdt_address;
        xsdt_address = 0;

        LOG(INFO, "RSDT address : 0x%lx", rsdt_address);

        // sdt_count = (rsdt->header.length - sizeof(struct sdt_header)) / 4;
        sdt_count = (table_read_member(struct rsdt_table, rsdt_address, header.length, true) - sizeof(struct sdt_header)) / 4;
    }
    else
    {
        uint64_t address = rsdp->xsdt_address;
        if (address >> 32)
        {
            LOG(CRITICAL, "64 bit XSDT address");
            abort();
        }

        xsdt_address = (physical_address_t)((uint32_t)address);
        rsdt_address = 0;

        LOG(INFO, "XSDT address : 0x%lx", xsdt_address);

        sdt_count = (table_read_member(struct xsdt_table, xsdt_address, header.length, true) - sizeof(struct sdt_header)) / 8;
    }

    LOG(INFO, "%u SDT tables detected", sdt_count);
    printf("%u SDT tables detected\n", sdt_count);

    for (uint32_t i = 0; i < sdt_count; i++)
    {
        uint64_t address = read_rsdt_ptr(i);
        LOG(INFO, "\tFound table at address 0x%lx", address);
        if (!(address >> 32))
        {
            if (acpi_table_valid(address))
            {
                uint32_t signature = table_read_member(struct sdt_header, address, signature, true);
                char signature_text[5] = { (char)signature, (char)(signature >> 8), (char)(signature >> 16), (char)(signature >> 24), 0 };
                LOG(INFO, "\t\tSignature: %s (0x%x)", signature_text, signature);
                printf("Signature: %s (0x%x)\n", signature_text, signature);
                switch (signature)
                {
                case 0x50434146:    // FACP : FADT
                    LOG(INFO, "\t\tValid FADT");
                    // printf("Found FADT\n");
                    fadt_address = address;
                    break;
                default:
                    LOG(INFO, "\t\tUnkwown table");
                }
            }
        }
    }
}

uint64_t read_rsdt_ptr(uint32_t index)
{
    if (index >= sdt_count)
    {
        LOG(CRITICAL, "Kernel tried to read an invalid SDT (%u / %u)", index + 1, sdt_count);
        abort();
    }

    if (acpi_10)    // ACPI 1.0
    {
        physical_address_t sdt_ptr_start = sizeof(struct sdt_header) + rsdt_address;
        return table_read_bytes(sdt_ptr_start, 4 * index, 4, true);
    }
    else            // ACPI 2.0+
    {
        physical_address_t sdt_ptr_start = sizeof(struct sdt_header) + rsdt_address;
        return table_read_bytes(sdt_ptr_start, 8 * index, 4, true) | ((uint64_t)table_read_bytes(sdt_ptr_start, 8 * index + 4, 4, true) << 32);
        // if (address >> 32)
        // {
        //     LOG(CRITICAL, "64 bit SDT address at index %u", index);
        //     abort();
        // }
        // return (uint32_t)address;
    }
}