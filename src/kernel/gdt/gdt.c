#include "gdt.h"
#include <string.h>
#include "../multitasking/task.h"

struct gdt_entry GDT[7];	// 5 + 2 for TSS
struct tss_entry TSS;

void setup_gdt_entry(struct gdt_entry* entry, physical_address_t base, uint32_t limit, uint8_t access_byte, uint8_t flags)
{
    entry->base_lo = (base & 0xffff);
    entry->base_mid = ((base >> 16) & 0xff);
    entry->base_hi = ((base >> 24) & 0xff);
    entry->limit_lo = (limit & 0xffff);
    entry->limit_hi = ((limit >> 16) & 0xf);
    entry->access_byte = access_byte;
    entry->flags = flags;
}

void setup_ssd_gdt_entry(struct gdt_entry* entry, physical_address_t base, uint32_t limit, uint8_t access_byte, uint8_t flags)
{
    setup_gdt_entry(entry, base & 0xffffffff, limit, access_byte, flags);
    *(uint64_t*)&entry[1] = (base >> 32) & 0xffffffff;
}

void install_gdt()
{
    load_gdt(sizeof(GDT) - 1, (uint64_t)&GDT);
}

void setup_gdt_tss()
{
    memset(&GDT[0], 0, sizeof(struct gdt_entry));       // NULL Descriptor
    setup_gdt_entry(&GDT[1], 0, 0xfffff, 0x9A, 0xA);    // Kernel mode code segment
    setup_gdt_entry(&GDT[2], 0, 0xfffff, 0x92, 0xC);    // Kernel mode data segment
    setup_gdt_entry(&GDT[3], 0, 0xfffff, 0xF2, 0xC);    // User mode data segment
    setup_gdt_entry(&GDT[4], 0, 0xfffff, 0xFA, 0xA);    // User mode code segment

    memset(&TSS, 0, sizeof(struct tss_entry));
    TSS.rsp0 = TASK_KERNEL_STACK_TOP_ADDRESS;
    setup_ssd_gdt_entry(&GDT[5], (physical_address_t)&TSS, sizeof(struct tss_entry) - 1, 0x89, 0);  // TSS

    install_gdt();
    load_tss();
}
