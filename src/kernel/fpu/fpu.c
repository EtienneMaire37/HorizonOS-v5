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

inline void fpu_save_state(fpu_state_t* s)
{
    if (!has_fpu) return;
    
    asm volatile("fxsave %0" : "=m" (*(fpu_state_t*)(((uint32_t)s & 0xfffffff0) + 16)) : : "memory");
}

inline void fpu_restore_state(fpu_state_t* s)
{
    if (!has_fpu) return;
    asm volatile("fxrstor %0" : : "m" (*(fpu_state_t*)(((uint32_t)s & 0xfffffff0) + 16)) : "memory");
}

inline void fpu_state_init(fpu_state_t* s)
{
    if (!has_fpu) return;
    
    int32_t offset = ((uint32_t)&s) & 0xf;
    memcpy((void*)((uint32_t)s->data + offset), &default_state, 512);
}

inline void copy_fpu_state(fpu_state_t* from, fpu_state_t* to)
{
    if (!has_fpu) return;

    int32_t offset_from = ((uint32_t)&from) & 0xf;
    int32_t offset_to = ((uint32_t)&to) & 0xf;

    memcpy((void*)((uint32_t)to->data + offset_to), (void*)((uint32_t)from->data + offset_from), 512);
}