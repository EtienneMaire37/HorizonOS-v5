#pragma once

struct interrupt_registers
{
    uint32_t cr3, cr2;
    uint32_t ds;
    uint32_t edi, esi, ebp;
    uint32_t handled_esp, ebx, edx, ecx, eax;
    uint32_t interrupt_number, error_code;
    uint32_t eip, cs, eflags;
} __attribute__((packed));

struct privilege_switch_interrupt_registers
{
    uint32_t cr3, cr2;
    uint32_t ds;
    uint32_t edi, esi, ebp;
    uint32_t handled_esp, ebx, edx, ecx, eax;
    uint32_t interrupt_number, error_code;
    uint32_t eip, cs, eflags;

    uint32_t esp, ss;   // Read Intel Manuals ->> Vol. 3A 7-13
} __attribute__((packed));

#define is_a_valid_function(symbol_type) ((symbol_type) == 'T' || (symbol_type) == 'R' || (symbol_type) == 't' || (symbol_type) == 'r')  

uint32_t current_cr3;

// ^ Changed to local variables
// uint32_t iret_cr3;
// bool flush_tlb;  

char* error_str[32] = 
{
    "DIVISION_OVERFLOW_ERROR",
    "DEBUG",
    "NON-MASKABLE_INTERRUPT",
    "BREAKPOINT",
    "OVERFLOW",
    "BOUND_RANGE_EXCEEDED",
    "INVALID_OPCODE",
    "DEVICE_NOT_AVAILABLE",
    "DOUBLE_FAULT",
    "COPROCESSOR_SEGMENT_OVERRUN",
    "INVALID_TSS",
    "SEGMENT_NOT_PRESENT",
    "STACK_SEGMENT_FAULT",
    "GENERAL_PROTECTION_FAULT",
    "PAGE_FAULT",
    "",
    "X87_FLOATING_POINT_EXCEPTION",
    "ALIGNMENT_CHECK",
    "MACHINE_CHECK",
    "SIMD_FLOATING_POINT_EXCEPTION",
    "VIRTUALIZATION_EXCEPTION",
    "CONTROL_PROTECTION_EXCEPTION",
    "",
    "",
    "",
    "",
    "",
    "",
    "HYPERVISOR_INJECTION_EXCEPTION",
    "VMM_COMMUNICATION_EXCEPTION",
    "SECURITY_EXCEPTION",
    ""
};

// char* seg_error_code[4] = 
// {
//     "GDT",
//     "IDT",
//     "LDT",
//     "IDT"
// };

char* get_error_message(uint32_t fault, uint32_t error_code)
{
    if (fault >= 32) return "";
    if (fault == 14)    // Page fault
    {
        if ((error_code >> 15) & 1)
            return "SGX_VIOLATION_EXCEPTION";
    }
    return error_str[fault];
}


struct initrd_file* kernel_symbols_file = NULL;
char* kernel_symbols_data = NULL;

void kernel_panic(struct privilege_switch_interrupt_registers* registers);
void print_kernel_symbol_name(uint32_t eip);
uint32_t interrupt_handler(struct privilege_switch_interrupt_registers* registers);