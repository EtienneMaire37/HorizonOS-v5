#pragma once

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

void __attribute__((optimize("O0"))) install_gdt()
{
    _gdtr.size = sizeof(GDT) - 1;
    _gdtr.address = (uint32_t)&GDT;

    load_gdt();
}