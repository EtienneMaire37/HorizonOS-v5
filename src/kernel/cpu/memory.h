#pragma once

void invlpg(uint64_t addr)
{
    asm volatile("invlpg [%0]" :: "r" (addr) : "memory");
}

void load_cr3(uint64_t addr)
{
    asm volatile("mov cr3, rax" :: "a" ((uint64_t)addr) : "memory");
}

uint64_t get_cr3()
{
    uint64_t val;
    asm volatile("mov rax, cr3" : "=a"(val));
    return val;
}