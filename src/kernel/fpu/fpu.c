#pragma once

#include "fpu.h"
#include "sse.h"

#include "../memalloc/page_frame_allocator.h"

static inline void fpu_init_defaults()
{
    xsave_area_size = get_xsave_area_size();
    xsave_area_pages = (xsave_area_size + 0xfff) / 0x1000;

    fpu_default_state = pfa_allocate_contiguous_pages(xsave_area_pages);
    fpu_init();
    fpu_save_state(fpu_default_state);
}

static inline void fpu_init()
{
    if (!has_fpu) return;
    asm volatile("fninit" ::: "memory");
}

static inline void fpu_save_state(uint8_t* s)
{
    if (!has_fpu) return;
    
    asm volatile("xsaves [rbx]" :: "a"(0xffffffffffffffff), "b"(s), "c"(0xffffffffffffffff));
}

static inline void fpu_restore_state(uint8_t* s)
{
    if (!has_fpu) return;

    asm volatile("xrstors [rbx]" :: "a"(0xffffffffffffffff), "b" (s), "c"(0xffffffffffffffff) : "memory");
}

static inline void fpu_state_init(uint8_t* s)
{
    if (!has_fpu) return;
    
    memcpy(s, fpu_default_state, xsave_area_size);
}

static inline uint8_t* fpu_state_create()
{
    uint8_t* data = pfa_allocate_contiguous_pages(xsave_area_pages);
    fpu_state_init(data);
    return data;
}