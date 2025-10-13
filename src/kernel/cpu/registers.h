#pragma once

uint32_t get_cr0()
{
    uint32_t eax;
    asm volatile("mov eax, cr0" : "=a" (eax));
    return eax;
}

void load_cr0(uint32_t cr0)
{
    asm volatile("mov cr0, eax" : : "a" (cr0));
}

uint32_t get_cr4()
{
    uint32_t eax;
    asm volatile("mov eax, cr4" : "=a" (eax));
    return eax;
}

void load_cr4(uint32_t cr4)
{
    asm volatile("mov cr4, eax" : : "a" (cr4));
}

extern uint32_t get_eflags();
extern void set_eflags(uint32_t value);