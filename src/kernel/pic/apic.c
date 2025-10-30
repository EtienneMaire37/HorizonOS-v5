#pragma once

#include "apic.h"

void apic_init()
{
    uint64_t apic_base_msr = rdmsr(IA32_APIC_BASE_MSR);
    uint64_t apic_base_address = apic_base_msr & 0xffffff000;
    lapic = (volatile struct local_apic_registers*)apic_base_address;
}

uint16_t apic_get_cpu_id()
{
    return (lapic->id_register >> 24) & 0xff;
}