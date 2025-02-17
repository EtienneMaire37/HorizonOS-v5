#pragma once

void task_init(struct task* _task, uint32_t eip, char* name)
{
    _task->stack = (uint8_t*)pfa_allocate_page();

    uint8_t* task_stack_top = _task->stack + 4096;

    struct interrupt_registers* registers = (struct interrupt_registers*)(task_stack_top - sizeof(struct interrupt_registers));
    
    registers->eip = eip;
    registers->cs = KERNEL_CODE_SEGMENT;
    registers->ds = KERNEL_DATA_SEGMENT;
    registers->eflags = 0x200; // Interrupts enabled (bit 9)
    registers->ss = KERNEL_DATA_SEGMENT;
    registers->esp = (uint32_t)task_stack_top;
    registers->handled_esp = registers->esp - 7 * 4; // sizeof(struct interrupt_registers);
    
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

    task_create_virtual_address_space(_task);
}

void task_destroy(struct task* _task)
{
    pfa_free_page((virtual_address_t)_task->stack);
}

void task_virtual_address_space_destroy(struct task* _task)
{
    for (uint16_t i = 0; i < 1024; i++)
    {
        if (_task->page_directory[i].present)
        {
            struct page_table_entry* pt = (struct page_table_entry*)physical_address_to_virtual(_task->page_directory[i].address << 12);

            for (uint16_t j = 0; j < 1024; j++)
            {
                if (pt[j].present)
                {
                    pfa_free_page(physical_address_to_virtual(pt[j].address << 12));
                }
            }

            pfa_free_page((virtual_address_t)pt);
        }
    }
    pfa_free_page((virtual_address_t)_task->page_directory);
}

void task_virtual_address_space_create_page_table(struct task* _task, uint16_t index)
{
    struct page_table_entry* pt = (struct page_table_entry*)pfa_allocate_page();

    init_page_table(pt);

    add_page_table(_task->page_directory, index, virtual_address_to_physical((uint32_t)pt), PAGING_SUPERVISOR_LEVEL, true);
}

void task_virtual_address_space_remove_page_table(struct task* _task, uint16_t index)
{
    struct page_table_entry* pt = (struct page_table_entry*)physical_address_to_virtual(_task->page_directory[index].address << 12);

    for (uint16_t i = 0; i < 1024; i++)
    {
        if (pt[i].present)
            pfa_free_page(physical_address_to_virtual(pt[i].address << 12));
    }

    pfa_free_page((virtual_address_t)pt);

    remove_page_table(_task->page_directory, index);
}

void task_virtual_address_space_create_page(struct task* _task, uint16_t pd_index, uint16_t pt_index, uint8_t user_supervisor, uint8_t read_write)
{
    if (!_task->page_directory[pd_index].present)
        task_virtual_address_space_create_page_table(_task, pd_index);

    struct page_table_entry* pt = (struct page_table_entry*)physical_address_to_virtual(_task->page_directory[pd_index].address << 12);

    set_page(pt, pt_index, virtual_address_to_physical(pfa_allocate_page()), user_supervisor, read_write);
}

void task_create_virtual_address_space(struct task* _task)
{
    _task->page_directory = (struct page_directory_entry_4kb*)pfa_allocate_page();

    init_page_directory(_task->page_directory);

    _task->registers->cr3 = virtual_address_to_physical((uint32_t)_task->page_directory);

    task_virtual_address_space_create_page_table(_task, 0);
    struct page_table_entry* first_mb_pt = (struct page_table_entry*)physical_address_to_virtual((physical_address_t)_task->page_directory[0].address << 12);

    for (uint16_t i = 0; i < 256; i++)
    {
        set_page(first_mb_pt, i, i * 0x1000, PAGING_SUPERVISOR_LEVEL, true);
    }

    for (uint16_t i = 0; i < 256; i++)
    {
        task_virtual_address_space_create_page_table(_task, i + 768);
        // LOG(DEBUG, "Created page table %u at 0x%x", i + 768, (uint32_t)_task->page_directory[i + 768].address << 12);
        for (uint16_t j = 0; j < 1024; j++)
        {
            struct virtual_address_layout layout;
            layout.page_directory_entry = i + 768;
            layout.page_table_entry = j;
            layout.page_offset = 0;
            uint32_t address = *(uint32_t*)&layout - 0xc0000000;
            struct page_table_entry* pt = (struct page_table_entry*)physical_address_to_virtual((physical_address_t)_task->page_directory[i + 768].address << 12);
            set_page(pt, j, address, PAGING_SUPERVISOR_LEVEL, true);
            // LOG(DEBUG, "Created page at 0x%x", address);
        }
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

    LOG(DEBUG, "Switched to task \"%s\" (pid = 0x%x) | registers : esp : 0x%x, 0x%x : end esp : 0x%x | eip : 0x%x", 
        current_task->name, current_task, current_task->registers->esp, current_task->registers->handled_esp, *registers, current_task->registers->eip);
    
    *registers = current_task->registers;
}

void task_a_main()
{
    while (true) asm("int 0xff" :: "a" ('A'));
}

void task_b_main()
{
    while (true) asm("int 0xff" :: "a" ('B'));
}