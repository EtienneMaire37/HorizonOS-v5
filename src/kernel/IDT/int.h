#pragma once

struct interrupt_registers
{
    uint32_t cr3, cr2;
    uint32_t ds;
    uint32_t edi, esi, ebp;
    uint32_t handled_esp, ebx, edx, ecx, eax;
    uint32_t interrupt_number, error_code;
    uint32_t eip, cs, eflags, esp, ss;
} __attribute__((packed));

uint32_t current_cr3;

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

char* seg_error_code[4] = 
{
    "GDT",
    "IDT",
    "LDT",
    "IDT"
};

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

void kernel_panic(struct interrupt_registers* params);
uint32_t interrupt_handler(struct interrupt_registers* params);