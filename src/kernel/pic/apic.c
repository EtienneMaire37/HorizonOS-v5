#pragma once

#include "apic.h"

void apic_init()
{
    uint64_t apic_base_msr = rdmsr(IA32_APIC_BASE_MSR);
    uint64_t apic_base_address = apic_base_msr & 0xffffff000;
    lapic = (volatile local_apic_registers_t*)apic_base_address;
}

uint8_t apic_get_cpu_id()
{
    return (lapic->id_register >> 24) & 0xff;
}

void lapic_send_eoi()
{
    lapic->end_of_interrupt_register = 0;
}

void lapic_set_spurious_interrupt_number(uint8_t int_num)
{
    uint32_t val = lapic->spurious_interrupt_vector_register;
    val &= 0xffffff00;
    val |= int_num;
    lapic->spurious_interrupt_vector_register = val;
}

void lapic_enable()
{
    lapic->spurious_interrupt_vector_register |= 0x100;
}

void lapic_disable()
{
    lapic->spurious_interrupt_vector_register &= ~0x100;
}

void lapic_set_tpr(uint8_t p)
{
    uint32_t val = lapic->task_priority_register;
    val &= 0xffffff00;
    val |= p;
    lapic->task_priority_register = val;
}