#pragma once

#define ISR_TASK_GATE           0b0101
#define ISR_INTERRUPT_GATE_16   0b0110 
#define ISR_TRAP_GATE_16        0b0111 
#define ISR_INTERRUPT_GATE_32   0b1110 
#define ISR_TRAP_GATE_32        0b1111

struct idt_descriptor
{
    uint16_t size;      // The size of the table in bytes subtracted by 1
    uint32_t address;   // The linear address of the IDT (not the physical address, paging applies).
} __attribute__((__packed__));

struct idt_descriptor _idtr;

struct idt_entry
{   
    uint16_t offset_lo           : 16;
    uint16_t segment_selector    : 16;
    uint8_t reserved             : 8;
    uint8_t gate_type            : 4;
    uint8_t zero                 : 1;
    uint8_t DPL                  : 2;
    uint8_t present              : 1;
    uint16_t offset_hi           : 16;
} __attribute__((__packed__));

struct idt_entry IDT[256] __attribute__((aligned(8)));  // "The base addresses of the IDT should be aligned on an 8-byte boundary to maximize performance of cache line fills."
                                                        // -- Intel manual vol 3A 7.10
                                                        
extern void load_idt();
extern uint32_t interrupt_table[256];  // ptrs to the isrs

void setup_idt_entry(struct idt_entry* entry, uint16_t segment, physical_address_t offset, uint8_t privilege, uint8_t type);
void install_idt();