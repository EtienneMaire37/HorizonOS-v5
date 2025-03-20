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

char* errorString[32] = 
{
    "DIVISION_BY_ZERO__ERROR",
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

void kernel_panic(struct interrupt_registers* params);
uint32_t interrupt_handler(struct interrupt_registers* params);