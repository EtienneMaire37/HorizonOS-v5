#pragma once

void fpu_init()
{
    if (!has_fpu) return;
    asm volatile ("fninit");
}

void fpu_save_state(fpu_state_t* fpu_state)
{
    if (!has_fpu) return;
    asm volatile ("fsave [eax]" :: "a"(fpu_state));
}

void fpu_restore_state(fpu_state_t* fpu_state)
{
    if (!has_fpu) return;
    asm volatile ("frstor [eax]" :: "a"(fpu_state));
}

void fpu_state_init(fpu_state_t* fpu_state)
{
    if (!has_fpu) return;
    
    // * "Sets the FPU control, status, tag, instruction pointer, and data pointer registers to their default states. 
    // * The FPU control word is set to 037FH (round to nearest, all exceptions masked, 64-bit precision). 
    // * The status word is cleared (no exception flags set, TOP is set to 0). 
    // * The data registers in the register stack are left unchanged, but they are all tagged as empty (11B). 
    // * Both the instruction and data pointers are cleared."

    fpu_state->control_word = 0x37f;
    fpu_state->status_word = 0;
    fpu_state->tag_word = 0xffff;
    fpu_state->instruction_ptr_offset = fpu_state->instruction_ptr_selector = 0;
    fpu_state->operand_ptr_offset = fpu_state->operand_ptr_selector = 0;
}