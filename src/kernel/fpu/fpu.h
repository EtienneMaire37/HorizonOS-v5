#pragma once

extern uint16_t fpu_test;

bool has_fpu;

uint8_t* fpu_default_state;
uint32_t xsave_area_size, xsave_area_pages;

static inline void fpu_init();
static inline void fpu_save_state(uint8_t* fpu_state);
static inline void fpu_restore_state(uint8_t* fpu_state);
static inline void fpu_state_init(uint8_t* fpu_state);