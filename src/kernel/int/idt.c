#include "idt.h"
#include "../pic/pic.h"
#include "../gdt/gdt.h"

#include <stdint.h>
#include <assert.h>

// * "The base addresses of the IDT should be aligned on an 8-byte boundary
// * to maximize performance of cache line fills."
// * -- Intel manual vol 3A 7.10
struct idt_entry IDT[256] __attribute__((aligned(8)));

void setup_idt_entry(struct idt_entry* entry, uint16_t segment, physical_address_t offset, uint8_t privilege, uint8_t type)
{
    assert(entry);
    entry->qword_lo = (((uint64_t)offset & 0xffff0000) << 32) | (1ULL << 47) | ((uint64_t)privilege << 45) | ((uint64_t)type << 40) | ((uint64_t)segment << 16) | (offset & 0xffff);
    entry->qword_hi = ((offset & 0xffffffff00000000) >> 32);
}

void install_idt()
{
    for(uint8_t i = 0; i < 32; i++)
        setup_idt_entry(&IDT[i], KERNEL_CODE_SEGMENT, interrupt_table[i], 0b00, ISR_TRAP_GATE_64);

    for(uint8_t i = 32; i < 32 + 16; i++)
        setup_idt_entry(&IDT[i], KERNEL_CODE_SEGMENT, interrupt_table[i], 0b00, ISR_INTERRUPT_GATE_64);  // PIC IRQs

    for(uint16_t i = 32 + 16; i < 256; i++)
        setup_idt_entry(&IDT[i], KERNEL_CODE_SEGMENT, interrupt_table[i], 0b00, ISR_INTERRUPT_GATE_64);

    setup_idt_entry(&IDT[0xf0], KERNEL_CODE_SEGMENT, interrupt_table[0xf0], 0b11, ISR_INTERRUPT_GATE_64); // System call

    pic_disable();
    load_idt(sizeof(IDT) - 1, (uint64_t)&IDT);
}
