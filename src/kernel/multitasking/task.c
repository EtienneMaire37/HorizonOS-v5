#pragma once

void task_init(struct task* _task, uint32_t eip, char* name)
{
    uint8_t* task_stack_top = _task->stack + 4096;

    struct interrupt_registers* registers = (struct interrupt_registers*)(task_stack_top - sizeof(struct interrupt_registers));
    
    registers->eip = eip;
    registers->cs = KERNEL_CODE_SEGMENT;
    registers->ds = KERNEL_DATA_SEGMENT;
    registers->eflags = 0x200; // Interrupts enabled (bit 9)
    registers->ss = KERNEL_DATA_SEGMENT;
    registers->esp = (uint32_t)task_stack_top;
    registers->handled_esp = registers->esp - 7 * 4; // sizeof(struct interrupt_registers);
    
    // Initialize other registers to 0
    registers->eax = registers->ebx = registers->ecx = registers->edx = 0;
    registers->esi = registers->edi = registers->ebp = 0;
    registers->cr3 = virtual_address_to_physical((uint32_t)page_directory);

    _task->registers = registers;
    size_t name_length = kstrlen(name);
    if (name_length >= 31)
    {
        kmemcpy(_task->name, name, 31);
        _task->name[31] = '\0';
    }
    else
    {
        kmemcpy(_task->name, name, name_length);
        _task->name[name_length] = '\0';
    }
}

// void task_init_from_memory(struct task* _task, void (*function)(), uint32_t size, char* name)
// {
//     task_init(_task, (uint32_t)function, name);
// }

void switch_task(struct interrupt_registers** registers)
{
    if (!first_task_switch) 
        current_task->registers = *registers;

    first_task_switch = false;
    
    // LOG(DEBUG, "Current registers : esp : 0x%x, 0x%x | eip : 0x%x", 
    //     registers->esp, registers->handled_esp, registers->eip);

    current_task = current_task->next_task;
    
    *registers = current_task->registers;

    LOG(DEBUG, "Switched to task \"%s\" (pid = 0x%x) | registers : esp : 0x%x, 0x%x | eip : 0x%x", 
        current_task->name, current_task, current_task->registers->esp, current_task->registers->handled_esp, current_task->registers->eip);
}

void task_a_main()
{
    while (true) asm("int 0xff" :: "a" ('A'));
}

void task_b_main()
{
    while (true) asm("int 0xff" :: "a" ('B'));
}