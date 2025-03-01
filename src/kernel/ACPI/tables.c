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
    for (int i = 0; i < header->length; i++)
        sum += ((uint8_t*)&header[0])[i];
    return sum == 0;
}

void acpi_find_tables()
{
    rsdp = rsdt = NULL;
}