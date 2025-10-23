#pragma once

typedef struct __attribute__((packed)) mbr_partition_table_entry
{
    uint8_t drive_attributes;   // 0x80: Bootable, 0x00: Non-bootable

    // CHS Address of partition start 
    uint8_t starting_head;
    uint8_t starting_sector : 6;
    uint16_t starting_cylinder : 10;

    uint8_t partition_type;

    // CHS address of last partition sector
    uint8_t ending_head;
    uint8_t ending_sector : 6;
    uint16_t ending_cylinder : 10;

    uint32_t start_lba;
    uint32_t size_in_sectors;
} mbr_partition_table_entry_t;

typedef struct __attribute__((packed)) mbr_boot_sector
{
    uint8_t boot_code[440];
    uint32_t disk_signature;
    uint16_t reserved;
    mbr_partition_table_entry_t partition_table[4];
    uint16_t boot_sector_signature;  // 0xAA55
} mbr_boot_sector_t;
