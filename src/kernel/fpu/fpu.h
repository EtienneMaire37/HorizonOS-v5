#pragma once

extern uint16_t fpu_test;

bool has_fpu;

typedef struct fpu_state 
{
    uint8_t data[512 + 16];
} __attribute__((packed)) fpu_state_t;

void fpu_init();
void fpu_save_state(fpu_state_t* fpu_state);
void fpu_restore_state(fpu_state_t* fpu_state);
void fpu_state_init(fpu_state_t* fpu_state);