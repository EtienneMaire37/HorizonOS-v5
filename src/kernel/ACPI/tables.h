#pragma once

struct rsdp_table
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;  // Deprecated since ACPI 2.0+

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct sdt_header
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_tableid[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

struct rsdt_table
{
    struct sdt_header header;
    // uint32_t ptrs_to_sdt[(header.length - sizeof(header)) / 4];
} __attribute__((packed));

struct xsdt_table
{
    struct sdt_header header;
    // uint64_t ptrs_to_sdt[(header.length - sizeof(header)) / 8];
} __attribute__((packed));
  
struct rsdp_table* rsdp;
struct rsdt_table* rsdt;
struct xsdt_table* xsdt;

bool acpi_10;
uint32_t sdt_count;
uint8_t* ebda;

void bios_get_ebda_pointer();

void acpi_find_tables();
bool acpi_table_valid();
uint32_t read_sdt_ptr(uint32_t index);