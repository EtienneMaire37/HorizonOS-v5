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
            result |= ((uint32_t)read_physical_address_1b(table_address + offset + i)) << (8 * i);
        }
        else
        {
            result <<= 8;
            result |= read_physical_address_1b(table_address + offset + i);
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
    for (uint32_t i = 0; i < length; i++)
        sum += table_read_bytes(table_address, i, 1, true);
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
    rsdt_address = 0; // xsdt_address = 0;
    fadt_address = madt_address = ssdt_address = dsdt_address = 0;
    
    LOG(INFO, "Searching for the RSDP");
    // printf("Searching for the RSDP\n");

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
    
    rsdp = NULL;
    sdt_count = 0;
    return;

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
        // xsdt_address = 0;

        LOG(INFO, "RSDT address : 0x%lx", rsdt_address);

        uint32_t header_length = table_read_member(struct rsdt_table, rsdt_address, header.length, true);
        printf("Header length : %u\n", header_length); // ? 4026597203 on VBox

        sdt_count = header_length <= sizeof(struct sdt_header) ? 0 : (header_length - sizeof(struct sdt_header)) / 4;
    }
    else
    {
        rsdt_address = physical_null;
        sdt_count = 0;
    }

    LOG(INFO, "%u SDT tables detected", sdt_count);
    printf("%u SDT tables detected\n", sdt_count);

    for (uint32_t i = 0; i < sdt_count; i++)
    {
        physical_address_t address = read_rsdt_ptr(i);
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
                    fadt_address = address;
                    break;
                case 0x43495041:    // APIC : MADT
                    LOG(INFO, "\t\tValid MADT");
                    madt_address = address;
                    break;
                case 0x54445344:    // DSDT : DSDT
                    LOG(INFO, "\t\tValid DSDT");
                    dsdt_address = address;
                    break;
                case 0x54445353:    // SSDT : SSDT
                    LOG(INFO, "\t\tValid SSDT");
                    ssdt_address = address;
                    break;
                default:
                    LOG(INFO, "\t\tUnkwown table");
                }
            }
        }
        else
        {
            LOG(WARNING, "\t\t64bit table address");
            printf("64bit table address\n");
        }
    }

    putchar('\n');

    fadt_extract_data();
}

physical_address_t read_rsdt_ptr(uint32_t index)
{
    if (index >= sdt_count)
    {
        LOG(CRITICAL, "Kernel tried to read an invalid SDT (%u / %u)", index + 1, sdt_count);
        abort();
        return physical_null;
    }

    if (acpi_10)    // ACPI 1.0
    {
        physical_address_t sdt_ptr_start = sizeof(struct sdt_header) + rsdt_address;
        return table_read_bytes(sdt_ptr_start, 4 * index, 4, true);
    }

    return physical_null;
}

void fadt_extract_data()
{
    preferred_power_management_profile = 0;

    if (fadt_address == physical_null)
    {
        LOG(DEBUG, "No FADT found");
        ps2_controller_connected = true;
        return;
    }

    LOG(DEBUG, "Extracting data from the FADT");

    ps2_controller_connected = acpi_10 ? true : (table_read_member(struct fadt_table, fadt_address, boot_architecture_flags, true) & 0b10) == 0b10;

    uint8_t _preferred_power_management_profile = table_read_member(struct fadt_table, fadt_address, preferred_power_management_profile, true);

    if (_preferred_power_management_profile > 7)
        preferred_power_management_profile = 0;
    else 
        preferred_power_management_profile = _preferred_power_management_profile;

    LOG(INFO, "Preferred power management profile : %s (%u)", _preferred_power_management_profile > 7 ? "Unknown" : preferred_power_management_profile_text[preferred_power_management_profile], _preferred_power_management_profile);
    if (preferred_power_management_profile != 0)
        printf("Preferred power management profile : %s (%u)\n", _preferred_power_management_profile > 7 ? "Unknown" : preferred_power_management_profile_text[preferred_power_management_profile], _preferred_power_management_profile);

    uint32_t _dsdt_address = table_read_member(struct fadt_table, fadt_address, dsdt_address, true);
    // LOG(DEBUG, "_dsdt_address : 0x%x", _dsdt_address);
    if (_dsdt_address != 0)
    {
        if (acpi_table_valid(_dsdt_address))
        {
            LOG(INFO, "Valid DSDT");
            // printf("Found valid DSDT\n");
            dsdt_address = _dsdt_address;
        }
    }
}
