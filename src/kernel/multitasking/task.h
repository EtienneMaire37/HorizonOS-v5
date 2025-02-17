#pragma once

struct interrupt_registers;

struct task
{
    char name[32];
    struct interrupt_registers* registers;
    struct task* next_task;
    struct task* previous_task;
    // uint8_t stack[4096];
    uint8_t* stack;
};

#define TASK_SWITCH_DELAY 30 // ms

uint8_t multitasking_counter = 0;

struct task* current_task;
bool multitasking_enabled = false;
volatile bool first_task_switch = true;

void task_init(struct task* _task, uint32_t eip, char* name);
void switch_task(struct interrupt_registers** registers);