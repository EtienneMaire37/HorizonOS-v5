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

extern uint32_t get_eflags();
extern void set_eflags(uint32_t value);