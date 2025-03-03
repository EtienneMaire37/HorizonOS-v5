#pragma once

struct gdt_decriptor
{
    uint16_t size;      // The size of the table in bytes subtracted by 1
    uint32_t address;   // The linear address of the GDT (not the physical address, paging applies).
} __attribute__((__packed__));

struct gdt_decriptor _gdtr;

struct gdt_entry
{   
    uint16_t limit_lo   : 16;
    uint16_t base_lo    : 16;
    uint8_t base_mid    : 8;
    uint8_t access_byte : 8;
    uint8_t limit_hi    : 4;
    uint8_t flags       : 4;
    uint8_t base_hi     : 8;
} __attribute__((__packed__));

struct tss_entry 
{
	uint16_t link;
	uint16_t reserved0;

	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint16_t ss0;      // The stack segment to load when changing to kernel mode.
	uint16_t reserved1;

	uint32_t esp1; 
	uint16_t ss1;
	uint16_t reserved2;

	uint32_t esp2;
	uint16_t ss2;
	uint16_t reserved3;

	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;

	uint16_t es;
	uint16_t reserved4;
	uint16_t cs;
	uint16_t reserved5;
	uint16_t ss;
	uint16_t reserved6;
	uint16_t ds;
	uint16_t reserved7;
	uint16_t fs;
	uint16_t reserved8;
	uint16_t gs;
	uint16_t reserved9;

	uint16_t ldtr;
	uint16_t reserved10;
	uint16_t reserved11;
	uint16_t iopb;

	uint32_t ssp;
} __attribute__((__packed__));

#define TSS_TYPE_16BIT_TSS_AVL  0x1
#define TSS_TYPE_16BIT_TSS_BSY  0x3
#define TSS_TYPE_32BIT_TSS_AVL  0x9
#define TSS_TYPE_32BIT_TSS_BSY  0xB
#define TSS_TYPE_LDT            0x2

#define KERNEL_CODE_SEGMENT 	0x08
#define KERNEL_DATA_SEGMENT 	0x10
#define USER_CODE_SEGMENT   	(0x18 | 3)
#define USER_DATA_SEGMENT   	(0x20 | 3)
#define TSS_SEGMENT   			0x28

struct gdt_entry GDT[6];
struct tss_entry TSS;

extern void load_gdt();
extern void load_tss();

void setup_gdt_entry(struct gdt_entry* entry, physical_address_t base, uint32_t limit, uint8_t access_byte, uint8_t flags);
void install_gdt();