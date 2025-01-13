#pragma once

struct interrupt_registers;

struct task
{
    struct interrupt_registers registers;
    // char name[32];
    struct task* next_task;
    struct task* previous_task;
    uint8_t stack[4096];
};

#define TASK_SWITCH_DELAY 20 // ms

uint8_t multitasking_counter = 0;

struct task* current_task;
bool multitasking_enabled = false;
bool first_task_switch = true;

struct task task_init(uint32_t eip);
void switch_task(struct interrupt_registers* registers);