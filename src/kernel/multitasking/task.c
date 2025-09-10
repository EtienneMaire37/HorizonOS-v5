#pragma once

#include "idle.h"
#include "vas.h"

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

void task_destroy(thread_t* task)
{
    LOG(DEBUG, "Destroying task \"%s\" (pid = %lu, ring = %u)", task->name, task->pid, task->ring);
    vas_free(task->cr3);
    utf32_buffer_destroy(&task->input_buffer);
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
    LOG(DEBUG, "Adding task \"%s\" from function", name);

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

    LOG(DEBUG, "Done");
}

void task_write_at_address_1b(thread_t* task, uint32_t address, uint8_t value)
{
    struct virtual_address_layout layout = *(struct virtual_address_layout*)&address;
    uint32_t pde = read_physical_address_4b(task->cr3 + 4 * layout.page_directory_entry);
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

            // float total = 100 - (.01f * 10000 * (float)tasks[0].stored_cpu_ticks / TASK_SWITCHES_PER_SECOND);
            uint16_t total = 10000 - (10000 * tasks[0].stored_cpu_ticks / TASK_SWITCHES_PER_SECOND);
            if (total <= 0) total = +0;
            if (total >= 10000) total = 10000;

            LOG(TRACE, "CPU usage:");
            LOG(TRACE, "total : %u.%u%u %%", total / 100, (total / 10) % 10, total % 10);
            for (uint16_t i = 0; i < task_count; i++)
            {
                uint16_t this_percentage = 10000 * tasks[i].stored_cpu_ticks / TASK_SWITCHES_PER_SECOND;
                LOG(TRACE, "%s── task %d : %u.%u%u %%\t[pid = %ld]%s", task_count - i > 1 ? "├" : "└", i, this_percentage / 100, (this_percentage / 10) % 10, this_percentage % 10, tasks[i].pid, i == 0 ? " (* idle task *)" : "");
            }
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
    if (index == current_task_index)
    {
        LOG(CRITICAL, "Kernel tried killing current task");
        abort();
    }
    if (index >= task_count || index == 0)
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
    if (current_task_index > index)
        current_task_index--;
    task_count--;
}