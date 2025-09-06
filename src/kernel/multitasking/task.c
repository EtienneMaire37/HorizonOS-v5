#pragma once

#include "idle.h"

thread_t task_create_empty()
{
    thread_t task;
    task.name = NULL;

    utf32_buffer_init(&task.input_buffer);
    task.reading_stdin = task.was_reading_stdin = false;

    task.cr3 = physical_null;
    task.ring = 0;

    task.esp = 0;

    task.pid = current_pid++;
    task.system_task = true;

    fpu_state_init(&task.fpu_state);

    task.current_cpu_ticks = 0;
    task.stored_cpu_ticks = 0;

    return task;
}

void task_destroy(thread_t* _task)
{
    abort();

    // LOG(DEBUG, "Destroying task \"%s\" (pid = %lu, ring = %u)", _task->name, _task->pid, _task->ring);
    // LOG(TRACE, "Freeing virtual address space");
    // task_virtual_address_space_destroy(_task);
    // if (_task->kernel_stack_phys)
    // {
    //     LOG(TRACE, "Freeing kernel stack at 0x%x", _task->kernel_stack_phys);
    //     pfa_free_physical_page(_task->kernel_stack_phys);
    // }
    // if (_task->stack_phys)
    // {
    //     LOG(TRACE, "Freeing stack at 0x%x", _task->stack_phys);
    //     pfa_free_physical_page(_task->stack_phys);
    // }
    // LOG(TRACE, "Freeing input buffer");
    // utf32_buffer_destroy(&_task->input_buffer);
}

void task_virtual_address_space_destroy(thread_t* _task)
{
    abort();

    for (uint16_t i = 0; i < 768; i++)
    {
        uint32_t pde = read_physical_address_4b(_task->cr3 + 4 * i);
        if (pde & 1)
        {
            physical_address_t pt_paddr = pde & 0xfffff000;

            for (uint16_t j = (i == 0 ? 256 : 0); j < (i == 767 ? 1021 : 1024); j++)
            {
                uint32_t pte = read_physical_address_4b(pt_paddr + 4 * j);

                if (pte & 1)
                    pfa_free_physical_page(pte & 0xfffff000);
            }
            pfa_free_physical_page(pt_paddr);
        }
    }
    pfa_free_physical_page(_task->cr3);
}

void task_virtual_address_space_create_page_table(thread_t* _task, uint16_t index)
{
    abort();

    physical_address_t pt_phys = pfa_allocate_physical_page();

    physical_init_page_table(pt_phys);

    physical_add_page_table(_task->cr3, index, pt_phys, PAGING_USER_LEVEL, true);
}

void task_virtual_address_space_remove_page_table(thread_t* _task, uint16_t index)
{
    abort();

    physical_address_t pt_phys = read_physical_address_4b(_task->cr3 + 4 * index) & 0xfffff000;

    for (uint16_t i = 0; i < 1024; i++)
    {
        if (read_physical_address_4b(pt_phys + 4 * i) & 1)
            pfa_free_physical_page(read_physical_address_4b(pt_phys + 4 * i) & 0xfffff000);
    }

    pfa_free_physical_page(pt_phys);

    physical_remove_page_table(_task->cr3, index);
}

physical_address_t task_virtual_address_space_create_page(thread_t* _task, uint16_t pd_index, uint16_t pt_index, uint8_t user_supervisor, uint8_t read_write)
{
    abort();

    uint32_t pde = read_physical_address_4b(_task->cr3 + 4 * pd_index);

    if (!(pde & 1))
        task_virtual_address_space_create_page_table(_task, pd_index);

    physical_address_t pt_phys = pde & 0xfffff000;

    physical_address_t page = pfa_allocate_physical_page();
    physical_set_page(pt_phys, pt_index, page, user_supervisor, read_write);
    return page;
}

void task_create_virtual_address_space(thread_t* _task)
{
    abort();

    // LOG(DEBUG, "Creating a new virtual address space...");

    // _task->cr3 = pfa_allocate_physical_page();

    // physical_init_page_directory(_task->cr3);

    // physical_add_page_table(_task->cr3, 1023, _task->cr3, PAGING_SUPERVISOR_LEVEL, true);

    // task_virtual_address_space_create_page_table(_task, 0);
    // physical_address_t first_mb_pt_address = read_physical_address_4b(_task->cr3) & 0xfffff000;
    // for (uint16_t i = 0; i < 256; i++)
    //     physical_set_page(first_mb_pt_address, i, i * 0x1000, PAGING_SUPERVISOR_LEVEL, true);

    // for (uint16_t i = 0; i < 255; i++)  // !!!!! DONT OVERRIDE RECURSIVE PAGING
    // {
    //     task_virtual_address_space_create_page_table(_task, i + 768);
    //     struct virtual_address_layout layout;
    //     layout.page_directory_entry = i + 768;
    //     layout.page_offset = 0;
    //     physical_address_t pt_address = read_physical_address_4b(_task->cr3 + 4 * layout.page_directory_entry) & 0xfffff000;
    //     for (uint16_t j = 0; j < 1024; j++)
    //     {
    //         layout.page_table_entry = j;
    //         uint32_t address = *(uint32_t*)&layout - 0xc0000000;
    //         physical_set_page(pt_address, layout.page_table_entry, address, PAGING_SUPERVISOR_LEVEL, true);
    //     }
    // }

    // task_virtual_address_space_create_page_table(_task, 767);   // Stacks and physical memory access

    // physical_address_t pt_address = read_physical_address_4b(_task->cr3 + 4 * 767) & 0xfffff000;
    // physical_set_page(pt_address, 1021, 0, PAGING_SUPERVISOR_LEVEL, true);
    // physical_set_page(pt_address, 1022, _task->kernel_stack_phys, PAGING_SUPERVISOR_LEVEL, true);
    // physical_set_page(pt_address, 1023, _task->stack_phys, PAGING_USER_LEVEL, true);

    // LOG(DEBUG, "Done.");
}

void multitasking_init()
{
    task_count = 0;
    current_task_index = 0;
    current_pid = 0;
    zombie_task_index = 0;

    multitasking_add_idle_task("idle");
}

void multitasking_start()
{
    fflush(stdout);
    multitasking_enabled = true;
    current_task_index = 0;

    idle_main();
    abort();    // !!! Critical error if eip somehow gets there (impossible)
}

void multitasking_add_idle_task(char* name)
{
    if (task_count >= MAX_TASKS)
    {
        LOG(CRITICAL, "Too many tasks");
        abort();
    }

    if (task_count != 0)
    {
        LOG(CRITICAL, "The kernel task must be the first one");
        abort();
    }

    thread_t task = task_create_empty();
    task.name = name;
    task.cr3 = (uint32_t)page_directory;

    tasks[task_count++] = task;
}

physical_address_t vas_create_empty()
{
    physical_address_t cr3 = pfa_allocate_physical_page();
    physical_init_page_directory(cr3);
    physical_add_page_table(cr3, 1023, cr3, PAGING_SUPERVISOR_LEVEL, true); // recursive paging

    physical_add_page_table(cr3, 0, pfa_allocate_physical_page(), PAGING_SUPERVISOR_LEVEL, true);

    physical_address_t first_mb_pt_address = read_physical_address_4b(cr3) & 0xfffff000;
    for (uint16_t i = 0; i < 256; i++)
        physical_set_page(first_mb_pt_address, i, i * 0x1000, PAGING_SUPERVISOR_LEVEL, true);

    for (uint16_t i = 0; i < 255; i++)  // !!!!! DONT OVERRIDE RECURSIVE PAGING
    {
        physical_add_page_table(cr3, i + 768, pfa_allocate_physical_page(), PAGING_SUPERVISOR_LEVEL, true);

        struct virtual_address_layout layout;
        layout.page_directory_entry = i + 768;
        layout.page_offset = 0;
        physical_address_t pt_address = read_physical_address_4b(cr3 + 4 * layout.page_directory_entry) & 0xfffff000;
        for (uint16_t j = 0; j < 1024; j++)
        {
            layout.page_table_entry = j;
            uint32_t address = *(uint32_t*)&layout - 0xc0000000;
            physical_set_page(pt_address, layout.page_table_entry, address, PAGING_SUPERVISOR_LEVEL, true);
        }
    }

    // * Stacks and physical memory access
    physical_add_page_table(cr3, 767, pfa_allocate_physical_page(), PAGING_SUPERVISOR_LEVEL, true);   

    physical_address_t pt_address = read_physical_address_4b(cr3 + 4 * 767) & 0xfffff000;
    physical_set_page(pt_address, physical_memory_page_index, 0, PAGING_SUPERVISOR_LEVEL, true);

    physical_set_page(pt_address, kernel_stack_page_index, pfa_allocate_physical_page(), PAGING_SUPERVISOR_LEVEL, true);

    for (int i = stack_page_index_start; i <= stack_page_index_end; i++)
        physical_set_page(pt_address, i, pfa_allocate_physical_page(), PAGING_USER_LEVEL, true);
        
    return cr3;
}

void task_stack_push(thread_t* task, uint32_t value)
{
    task->esp -= 4;
    task_write_at_address_1b(task, (physical_address_t)task->esp + 0, (value >> 0)  & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->esp + 1, (value >> 8)  & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->esp + 2, (value >> 16) & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->esp + 3, (value >> 24) & 0xff);
}

void multitasking_add_task_from_function(char* name, void (*func)())
{
    thread_t task = task_create_empty();
    task.name = name;
    task.cr3 = vas_create_empty();

    task.esp = TASK_STACK_TOP_ADDRESS;

    task_stack_push(&task, 0x200);
    task_stack_push(&task, KERNEL_CODE_SEGMENT);
    task_stack_push(&task, (uint32_t)func);


    task_stack_push(&task, (uint32_t)iret_instruction);

    task_stack_push(&task, 0);   // ebx
    task_stack_push(&task, 0);   // esi
    task_stack_push(&task, 0);   // edi
    task_stack_push(&task, TASK_STACK_TOP_ADDRESS); // ebp

    tasks[task_count++] = task;
}

void task_write_at_address_1b(thread_t* _task, uint32_t address, uint8_t value)
{
    struct virtual_address_layout layout = *(struct virtual_address_layout*)&address;
    uint32_t pde = read_physical_address_4b(_task->cr3 + 4 * layout.page_directory_entry);
    if (!(pde & 1)) return;
    physical_address_t pt_address = (physical_address_t)pde & 0xfffff000;
    uint32_t pte = read_physical_address_4b(pt_address + 4 * layout.page_table_entry);
    if (!(pte & 1)) return;
    physical_address_t page_address = (physical_address_t)pte & 0xfffff000;
    physical_address_t byte_address = page_address | layout.page_offset;
    write_physical_address_1b(byte_address, value);
}

void switch_task(struct privilege_switch_interrupt_registers** registers)
{
    tasks[current_task_index].current_cpu_ticks++;

    global_cpu_ticks++;
    if (global_cpu_ticks >= TASK_SWITCHES_PER_SECOND)
    {
        if (task_count != 0)
        {
            for (uint16_t i = 0; i < task_count; i++)
            {
                tasks[i].stored_cpu_ticks = tasks[i].current_cpu_ticks;
                tasks[i].current_cpu_ticks = 0;
            }

            float total = 100 - (.01 * 10000 * tasks[0].stored_cpu_ticks / TASK_SWITCHES_PER_SECOND);
            if (total <= 0) total = +0;
            if (total >= 100) total = 100;

            LOG(TRACE, "CPU usage:");
            LOG(TRACE, "total : %f %%", total);
            for (uint16_t i = 0; i < task_count; i++)
                LOG(TRACE, "%s── task %d : %f %%\t[pid = %ld]%s", task_count - i > 1 ? "├" : "└", i, .01 * 10000 * tasks[i].stored_cpu_ticks / TASK_SWITCHES_PER_SECOND, tasks[i].pid, i == 0 ? " (* idle task *)" : "");
        }
        global_cpu_ticks = 0;
    }

    if (task_count == 0)
    {
        LOG(CRITICAL, "No task to switch to");
        abort();
    }

    bool was_first_task_switch = first_task_switch;
    first_task_switch = false;

    // return;

    uint16_t next_task_index = find_next_task_index();
    // LOG(TRACE, "Current task index : %u; Next task index : %u", current_task_index, next_task_index);
    if (next_task_index == current_task_index)
        return;

    fpu_save_state(&tasks[current_task_index].fpu_state);

    // LOG(TRACE, "Switching from task \"%s\" (pid = %lu, ring = %u) | registers : esp : 0x%x : end esp : 0x%x | ebp : 0x%x | eip : 0x%x, cs : 0x%x, eflags : 0x%x, ds : 0x%x, eax : 0x%x, ebx : 0x%x, ecx : 0x%x, edx : 0x%x, esi : 0x%x, edi : 0x%x, cr3 : 0x%x", 
    //     tasks[current_task_index].name, tasks[current_task_index].pid, tasks[current_task_index].ring, 
    //     (*registers)->handled_esp, tasks[current_task_index].esp, (*registers)->ebp,
    //     (*registers)->eip, (*registers)->cs, (*registers)->eflags, (*registers)->ds,
    //     (*registers)->eax, (*registers)->ebx, (*registers)->ecx, (*registers)->edx, (*registers)->esi, (*registers)->edi,
    //     (*registers)->cr3);

    full_context_switch(next_task_index);

    // LOG(TRACE, "Switched to task \"%s\" (pid = %lu, ring = %u) | registers : esp : 0x%x : end esp : 0x%x | ebp : 0x%x | eip : 0x%x, cs : 0x%x, eflags : 0x%x, ds : 0x%x, eax : 0x%x, ebx : 0x%x, ecx : 0x%x, edx : 0x%x, esi : 0x%x, edi : 0x%x, cr3 : 0x%x", 
    //     tasks[current_task_index].name, tasks[current_task_index].pid, tasks[current_task_index].ring, 
    //     (*registers)->handled_esp, tasks[current_task_index].esp, (*registers)->ebp,
    //     (*registers)->eip, (*registers)->cs, (*registers)->eflags, (*registers)->ds,
    //     (*registers)->eax, (*registers)->ebx, (*registers)->ecx, (*registers)->edx, (*registers)->esi, (*registers)->edi,
    //     (*registers)->cr3);

    // *flush_tlb = true;

    // if (tasks[current_task_index].ring == 3)
    //     TSS.esp0 = TASK_KERNEL_STACK_TOP_ADDRESS;

    // // *iret_cr3 = tasks[current_task_index].registers_data.cr3;
    // // *registers = tasks[current_task_index].registers_ptr;       // * When poping esp load the correct values

    // // if (tasks[current_task_index].was_reading_stdin)
    // // {
    // //     uint32_t _eax = minint(get_buffered_characters(tasks[current_task_index].input_buffer), tasks[current_task_index].registers_data.edx);
    // //     task_write_register_data(&tasks[current_task_index], eax, _eax);
    // //     for (uint32_t i = 0; i < _eax; i++)
    // //     {
    // //         // *** Only ASCII for now ***
    // //         task_write_at_address_1b(&tasks[current_task_index], tasks[current_task_index].registers_data.ecx + i, utf32_to_bios_oem(utf32_buffer_getchar(&tasks[current_task_index].input_buffer)));
    // //     }
    // // }

    // tasks[current_task_index].was_reading_stdin = false;

    fpu_restore_state(&tasks[current_task_index].fpu_state);
}

void task_kill(uint16_t index)
{
    abort();

    if (index >= task_count || index == 0 || index == current_task_index)
    {
        LOG(CRITICAL, "Invalid task index %u", index);
        abort();
    }
    if (task_count == 1)
    {
        LOG(CRITICAL, "Zero tasks remaining");
        abort();
    }

    task_destroy(&tasks[index]);
    for (uint16_t i = index; i < task_count - 1; i++)
        tasks[i] = tasks[i + 1];
    if (current_task_index >= index)
        current_task_index--;
    task_count--;
}