#pragma once

#include <stdint.h>
#include "../cpu/util.h"

#define ISR_INTERRUPT_GATE_64   0b1110
#define ISR_TRAP_GATE_64        0b1111

struct idt_entry
{
    uint64_t qword_lo;
    uint64_t qword_hi;
} __attribute__((packed));

extern struct idt_entry IDT[256];

extern void load_idt(uint16_t limit, uint64_t address);
extern uint64_t interrupt_table[256];  // ptrs to the isrs

void setup_idt_entry(struct idt_entry* entry, uint16_t segment, physical_address_t offset, uint8_t privilege, uint8_t type);
void install_idt();
