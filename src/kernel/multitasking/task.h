#pragma once

struct task
{
    struct interrupt_registers registers;
    // char name[32];
    struct task* next_task, *previous_task;
};

struct task* current_task;