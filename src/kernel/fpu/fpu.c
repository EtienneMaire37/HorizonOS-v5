#pragma once

void fpu_init(void)
{
    if (!has_fpu) return;
    asm volatile("fninit" ::: "memory");
}

void fpu_save_state(fpu_state_t* s)
{
    if (!has_fpu) return;
    
    asm volatile("fxsave %0" : "=m" (*(fpu_state_t*)(((uint32_t)s & 0xfffffff0) + 16)) : : "memory");
}

void fpu_restore_state(fpu_state_t* s)
{
    if (!has_fpu) return;
    asm volatile("fxrstor %0" : : "m" (*(fpu_state_t*)(((uint32_t)s & 0xfffffff0) + 16)) : "memory");
}

void fpu_state_init(fpu_state_t* s)
{
    if (!has_fpu) return;
    
    fpu_init();
    fpu_save_state(s);
}