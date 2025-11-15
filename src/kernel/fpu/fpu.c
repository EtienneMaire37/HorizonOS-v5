#pragma once

#include "fpu.h"
#include "sse.h"

#include "../memalloc/page_frame_allocator.h"

static inline void fpu_init_defaults()
{
    xsave_area_size = get_xsave_area_size();
    xsave_area_pages = (xsave_area_size + 0xfff) / 0x1000;

    fpu_default_state = pfa_allocate_contiguous_pages(xsave_area_pages);
    memset((uint8_t*)fpu_default_state, 0, xsave_area_size);
    fpu_init();
    fpu_save_state(fpu_default_state);

// !!! The XSAVE instruction does not write any part of the XSAVE header other than the XSTATE_BV field; in particular,
// !!! it does *not* write to the XCOMP_BV field.
}

static inline void fpu_init()
{
    if (!has_fpu) return;
    asm volatile("fninit" ::: "memory");
}

static inline void fpu_save_state(volatile uint8_t* s)
{
    if (!has_fpu) return;

    asm volatile("xsave [rdi]" :: "a"(STATE_COMPONENT_BITMAP & 0xffffffff), "D"(s), "d"(STATE_COMPONENT_BITMAP >> 32) : "memory");
}

static inline void fpu_restore_state(volatile uint8_t* s)
{
    if (!has_fpu) return;

    asm volatile("xrstor [rdi]" :: "a"(STATE_COMPONENT_BITMAP & 0xffffffff), "D" (s), "d"(STATE_COMPONENT_BITMAP >> 32) : "memory");
}

static inline void fpu_state_init(volatile uint8_t* s)
{
    if (!has_fpu) return;
    
    memcpy((uint8_t*)s, (uint8_t*)fpu_default_state, xsave_area_size);
}

static inline volatile uint8_t* fpu_state_create_empty()
{
    // * Alignment is needed
    volatile uint8_t* ptr = pfa_allocate_contiguous_pages(xsave_area_pages);
    memset((uint8_t*)ptr, 0, xsave_area_size);
    // LOG(DEBUG, "Allocated FPU state: %p", ptr);
    return ptr;
}

static inline volatile uint8_t* fpu_state_create()
{
    volatile uint8_t* data = fpu_state_create_empty();
    fpu_state_init(data);
    return data;
}

static inline volatile uint8_t* fpu_state_create_copy(volatile uint8_t* state)
{
    volatile uint8_t* data = fpu_state_create_empty();
    memcpy((uint8_t*)data, (uint8_t*)state, xsave_area_size);
    return data;
}

static inline void fpu_state_destroy(volatile uint8_t** data)
{
    if (!data) return;
    if (!(*data)) return;

    // LOG(DEBUG, "Freeing FPU state: %p", *data);

    pfa_free_contiguous_pages((void*)*data, xsave_area_pages);
    (*data) = NULL;
}