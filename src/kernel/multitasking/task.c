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

    task.is_dead = false;
    task.forked_pid = 0;    // is_being_forked = false

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

void task_stack_push_data(thread_t* task, void* data, size_t bytes)
{
    task->esp -= bytes;
    for (int i = 0; i < bytes; i++)
        task_write_at_address_1b(task, (physical_address_t)task->esp + i, ((uint8_t*)data)[i]);
}

void task_stack_push_string(thread_t* task, const char* str)
{
    const int bytes = strlen(str) + 1;
    task_stack_push_data(task, (void*)str, bytes);
}

void task_write_at_address_1b(thread_t* task, uint32_t address, uint8_t value)
{
    if (task->cr3 == physical_null)
    {
        LOG(WARNING, "Kernel tried to write into a null vas");
        return;
    }
    
    uint32_t pde = read_physical_address_4b(task->cr3 + 4 * (address >> 22));
    if (!(pde & 1)) return;
    physical_address_t pt_address = pde & 0xfffff000;
    uint32_t pte = read_physical_address_4b(pt_address + 4 * ((address >> 12) & 0x3ff));
    if (!(pte & 1)) return;
    physical_address_t page_address = pte & 0xfffff000;
    physical_address_t byte_address = page_address | (address & 0xfff);
    write_physical_address_1b(byte_address, value);
}

void switch_task()
{
    if (task_count == 0)
    {
        LOG(CRITICAL, "No task to switch to");
        abort();
    }

    lock_task_queue();

    bool was_first_task_switch = first_task_switch;
    first_task_switch = false;

    uint16_t next_task_index = find_next_task_index();
    if (tasks[current_task_index].pid != tasks[next_task_index].pid)
    {
        fpu_save_state(&tasks[current_task_index].fpu_state);

        full_context_switch(next_task_index);

        fpu_restore_state(&tasks[current_task_index].fpu_state);
    }

    cleanup_tasks();

    unlock_task_queue();
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
    {
        tasks[i] = tasks[i + 1];
        copy_fpu_state(&tasks[i + 1].fpu_state, &tasks[i].fpu_state);
    }
    if (current_task_index > index)
        current_task_index--;
    task_count--;
}

void copy_task(uint16_t index)
{
    if (index >= task_count || index == 0)
    {
        LOG(CRITICAL, "Invalid task index %u", index);
        abort();
    }

    task_count++;

    const uint16_t new_task_index = task_count - 1;

    tasks[new_task_index].name = tasks[index].name;
    tasks[new_task_index].ring = tasks[index].ring;
    tasks[new_task_index].pid = tasks[index].forked_pid;
    tasks[new_task_index].system_task = tasks[index].system_task;
    tasks[new_task_index].reading_stdin = false;
    utf32_buffer_copy(&tasks[index].input_buffer, &tasks[new_task_index].input_buffer);

    tasks[new_task_index].cr3 = vas_create_empty();
    tasks[new_task_index].esp = tasks[index].esp;

    copy_fpu_state(&tasks[index].fpu_state, &tasks[new_task_index].fpu_state);

    tasks[new_task_index].forked_pid = 0;
    tasks[new_task_index].is_dead = false;
    
    for (uint16_t i = 0; i < 768; i++)
    {
        uint32_t old_pde = read_physical_address_4b(tasks[index].cr3 + 4 * i);
        uint32_t new_pde = read_physical_address_4b(tasks[new_task_index].cr3 + 4 * i);
        if (!(old_pde & 1)) continue;
        physical_address_t old_pt_address = old_pde & 0xfffff000;
        physical_address_t new_pt_address = new_pde & 0xfffff000;
        if (!(new_pde & 1))
        {
            new_pt_address = pfa_allocate_physical_page();
            physical_init_page_table(new_pt_address);
            write_physical_address_4b(tasks[new_task_index].cr3 + 4 * i, new_pt_address | (old_pde & 0xfff));
        }
        // LOG(TRACE, "%u : old_pt_address : 0x%lx", i, old_pt_address);
        // LOG(TRACE, "%u : new_pt_address : 0x%lx", i, new_pt_address);
        for (uint16_t j = (i == 0 ? 256 : 0); j < 1024; j++)
        {
            uint32_t old_pte = read_physical_address_4b(old_pt_address + 4 * j);
            physical_address_t old_page_address = old_pte & 0xfffff000;
            if (old_pte & 1)
            {
                uint32_t new_pte = read_physical_address_4b(new_pt_address + 4 * j);
                physical_address_t new_page_address = new_pte & 0xfffff000;
                if (!(new_pte & 1))
                {
                    new_page_address = pfa_allocate_physical_page();
                    write_physical_address_4b(new_pt_address + 4 * j, new_page_address | (old_pte & 0xfff));
                }
                // LOG(TRACE, "%u.%u : old_page_address : 0x%lx", i, j, old_page_address);
                // LOG(TRACE, "%u.%u : new_page_address : 0x%lx", i, j, new_page_address);
                copy_page(old_page_address, new_page_address);
            }
        }
    }
}

void cleanup_tasks()
{
    for (uint16_t i = 0; i < task_count; i++)
    {
        if (i == current_task_index) continue;
        if (tasks[i].forked_pid)
        {
            copy_task(i);
            tasks[i].forked_pid = 0;
        }
    }

    for (uint16_t i = 0; i < task_count; i++)
    {
        if (i == current_task_index) continue;
        if (tasks[i].is_dead)
        {
            task_kill(i);
            i--;
            continue;
        }
    }
}