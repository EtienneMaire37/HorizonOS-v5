#pragma once

struct task task_init(uint32_t eip)
{
    struct task _task;
    _task.registers.eip = eip;
    _task.registers.cs = KERNEL_CODE_SEGMENT;
    _task.registers.ss = KERNEL_DATA_SEGMENT;
    // _task.registers.cr0 = ((uint32_t)1 << 31) | 1;  // Enable paging, protected mode
    // _task.registers.cr3 = virtual_address_to_physical((uint32_t)&page_directory);
    _task.registers.eflags = ((uint32_t)1 << 9);  // Enable interrupts
    _task.registers.ebp = (uint32_t)_task.stack + 4096 - sizeof(struct interrupt_registers);
    _task.registers.esp = _task.registers.ebp;
    _task.registers.current_esp = _task.registers.ebp;// + sizeof(struct interrupt_registers);
    return _task;
}

void switch_task(struct interrupt_registers* registers)
{
    if (!first_task_switch)
    {
        current_task->registers = *registers;
        first_task_switch = false;
    }
    current_task = current_task->next_task;
    current_task->registers.cr3 = registers->cr3;
    *registers = current_task->registers;
    LOG(DEBUG, "Switched to task 0x%x registers: eip: 0x%x, esp: 0x%x, current esp: 0x%x, ebp: 0x%x", 
    current_task, current_task->registers.eip, current_task->registers.esp, current_task->registers.current_esp, current_task->registers.ebp);
}

void task_a_main()
{
    while (true) kputchar('A');
}

void task_b_main()
{
    while (true) kputchar('B');
}