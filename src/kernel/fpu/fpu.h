#pragma once

extern uint16_t fpu_test;

bool has_fpu;

uint8_t* fpu_default_state = NULL;
uint32_t xsave_area_size, xsave_area_pages;

void fpu_init();
void fpu_save_state(uint8_t* fpu_state);
void fpu_restore_state(uint8_t* fpu_state);
void fpu_state_init(uint8_t* fpu_state);