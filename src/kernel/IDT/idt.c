#pragma once

void setup_idt_entry(struct idt_entry* entry, uint16_t segment, physical_address_t offset, uint8_t privilege, uint8_t type)
{
    entry->segment_selector = segment;

    entry->offset_lo = (offset & 0xffff);
    entry->offset_hi = ((offset >> 16) & 0xffff);

    entry->DPL = privilege;
    entry->gate_type = type;

    entry->present = 1; // Gate is present
    entry->zero = 0;
}

void install_idt()
{
    for(uint8_t i = 0; i < 32; i++)
        setup_idt_entry(&IDT[i], KERNEL_CODE_SEGMENT, interrupt_table[i], 0b00, ISR_INTERRUPT_GATE_32);

    for(uint8_t i = 32; i < 32 + 16; i++)
        setup_idt_entry(&IDT[i], KERNEL_CODE_SEGMENT, interrupt_table[i], 0b00, ISR_INTERRUPT_GATE_32);

    for(uint16_t i = 32 + 16; i < 255; i++)
        setup_idt_entry(&IDT[i], KERNEL_CODE_SEGMENT, interrupt_table[i], 0b00, ISR_INTERRUPT_GATE_32);

    setup_idt_entry(&IDT[255], KERNEL_CODE_SEGMENT, interrupt_table[255], 0b11, ISR_INTERRUPT_GATE_32); // System call

    _idtr.size = sizeof(IDT) - 1;
    _idtr.address = (uint32_t)&IDT;

    pic_disable();
    load_idt();
}