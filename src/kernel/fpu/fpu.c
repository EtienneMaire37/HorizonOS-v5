#pragma once

static fpu_state_t default_state __attribute__((aligned(16)));

void fpu_init_defaults()
{
    assert(((virtual_address_t)&default_state & 0xf) == 0);
    fpu_init();
    fpu_save_state(&default_state);
}

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
    
    int32_t offset = ((uint32_t)&s) & 0xf;
    memcpy((void*)((uint32_t)s->data + offset), &default_state, 512);
}