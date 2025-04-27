#pragma once

extern uint16_t fpu_test;

bool has_fpu;

typedef struct fpu_state 
{
	uint32_t control_word;          // ~ Top bits are 0
	uint32_t status_word;           // ~ Same here 
	uint32_t tag_word;              // ~ Same here // *** 2bits per register, 8 registers/10bytes per register
	uint32_t instruction_ptr_offset;
	uint32_t instruction_ptr_selector;
	uint32_t operand_ptr_offset;
	uint32_t operand_ptr_selector;

	uint8_t	stack[80];

	uint32_t status;
} __attribute__((packed)) fpu_state_t;

void fpu_init();
void fpu_save_state(fpu_state_t* fpu_state);
void fpu_restore_state(fpu_state_t* fpu_state);
void fpu_state_init(fpu_state_t* fpu_state);