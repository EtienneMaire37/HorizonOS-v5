#pragma once

struct task task_init(uint32_t eip)
{
    struct task task;
    task.registers.eip = eip;
    task.registers.cs = KERNEL_CODE_SEGMENT;
    task.registers.ss = KERNEL_DATA_SEGMENT;
    task.registers.cr0 = ((uint32_t)1 << 31) | 1;  // Enable paging, protected mode
    task.registers.eflags = ((uint32_t)1 << 9);  // Enable interrupts
    return task;
}

struct task switch_task(struct interrupt_registers registers)
{
    ;
}