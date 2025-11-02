#pragma once

#include "idle.h"
#include "vas.h"

void task_init_file_table(thread_t* task)
{
    for (int i = 0; i < OPEN_MAX; i++)
        task->file_table[i] = invalid_fd;
    task->file_table[0] = 0;    // * STDIN_FILENO
    task->file_table[1] = 1;    // * STDOUT_FILENO
    task->file_table[2] = 2;    // * STDERR_FILENO
}

thread_t task_create_empty()
{
    thread_t task;

    memset(task.name, 0, THREAD_NAME_MAX);

    utf32_buffer_init(&task.input_buffer);
    task.reading_stdin = false;

    task.cr3 = physical_null;
    task.ring = 0;

    task.rsp = 0;

    task.pid = current_pid++;
    task.system_task = true;

    fpu_state_init(&task.fpu_state);

    task.current_cpu_ticks = 0;
    task.stored_cpu_ticks = 0;

    task.is_dead = false;
    task.forked_pid = 0;    // is_being_forked = false

    task.parent = -1;
    task.wait_pid = -1;

    task.to_reap = false;

    task_init_file_table(&task);

    return task;
}

void task_destroy(thread_t* task)
{
    // LOG(DEBUG, "Destroying task \"%s\" (pid = %d, ring = %u)", task->name, task->pid, task->ring);
    // vas_free(task->cr3);
    // utf32_buffer_destroy(&task->input_buffer);
    // for (int i = 0; i < OPEN_MAX; i++)
    // {
    //     if (task->file_table[i] != invalid_fd)
    //         vfs_remove_global_file(task->file_table[i]);
    // }
    abort();
}

void multitasking_init()
{
    task_count = 0;
    current_task_index = 0;
    current_pid = 0;

    vfs_init_file_table();

    multitasking_add_idle_task("idle");
}

void multitasking_start()
{
    fflush(stdout);
    multitasking_enabled = true;
    current_task_index = 0;

    idle_main();
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
    int name_bytes = minint(strlen(name), THREAD_NAME_MAX - 1);
    memcpy(task.name, name, name_bytes);
    task.name[name_bytes] = 0;
    task.cr3 = get_cr3();

    tasks[task_count++] = task;
}

void task_stack_push(thread_t* task, uint64_t value)
{
    task->rsp -= 8;

    task_write_at_address_1b(task, (physical_address_t)task->rsp + 0, (value >> 0)  & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->rsp + 1, (value >> 8)  & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->rsp + 2, (value >> 16) & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->rsp + 3, (value >> 24) & 0xff);

    task_write_at_address_1b(task, (physical_address_t)task->rsp + 4, (value >> 32) & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->rsp + 5, (value >> 40) & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->rsp + 6, (value >> 48) & 0xff);
    task_write_at_address_1b(task, (physical_address_t)task->rsp + 7, (value >> 56) & 0xff);
}

void task_stack_push_data(thread_t* task, void* data, size_t bytes)
{
    task->rsp -= bytes;
    for (size_t i = 0; i < bytes; i++)
        task_write_at_address_1b(task, (physical_address_t)task->rsp + i, ((uint8_t*)data)[i]);
}

void task_stack_push_string(thread_t* task, const char* str)
{
    const int bytes = strlen(str) + 1;
    task_stack_push_data(task, (void*)str, bytes);
}

static inline void task_write_at_address_1b(thread_t* task, uint64_t address, uint8_t value)
{
    if (task->cr3 == physical_null)
    {
        LOG(WARNING, "Kernel tried to write into a null vas");
        return;
    }
    
    uint8_t* ptr = (uint8_t*)virtual_to_physical((uint64_t*)task->cr3, address);
    *ptr = value;
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
        // fpu_save_state(&tasks[current_task_index].fpu_state);

        full_context_switch(next_task_index);

        // fpu_restore_state(&tasks[current_task_index].fpu_state);
    }

    cleanup_tasks();

    unlock_task_queue();
}

thread_t* find_task_by_pid(pid_t pid)
{
    for (uint16_t i = 0; i < task_count; i++)
        if (tasks[i].pid == pid)
            return &tasks[i];
    return NULL;
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

void task_copy_file_table(uint16_t from, uint16_t to, bool cloexec)
{
    acquire_spinlock(&file_table_spinlock);
    for (int i = 3; i < OPEN_MAX; i++)
    {
        if (cloexec && tasks[from].file_table[i] == invalid_fd && (file_table[tasks[to].file_table[i]].flags & O_CLOEXEC))
            tasks[to].file_table[i] = invalid_fd;
        else
        {
            tasks[to].file_table[i] = tasks[from].file_table[i];
            if (tasks[to].file_table[i] != invalid_fd)
                file_table[tasks[to].file_table[i]].used++;
        }
    }
    release_spinlock(&file_table_spinlock);
}

void copy_task(uint16_t index)
{
    if (index >= task_count || index == 0)
    {
        LOG(CRITICAL, "Invalid task index %u", index);
        abort();
        return;
    }

    task_count++;

    const uint16_t new_task_index = task_count - 1;

    tasks[new_task_index] = tasks[index];

    // tasks[new_task_index].name = tasks[index].name;
    memcpy(tasks[new_task_index].name, tasks[index].name, THREAD_NAME_MAX);
    tasks[new_task_index].ring = tasks[index].ring;
    tasks[new_task_index].pid = tasks[index].forked_pid;
    tasks[new_task_index].system_task = tasks[index].system_task;
    tasks[new_task_index].reading_stdin = false;
    utf32_buffer_copy(&tasks[index].input_buffer, &tasks[new_task_index].input_buffer);

    tasks[new_task_index].cr3 = (uint64_t)task_create_empty_vas();
    tasks[new_task_index].rsp = tasks[index].rsp;

    copy_fpu_state(&tasks[index].fpu_state, &tasks[new_task_index].fpu_state);

    task_copy_file_table(index, new_task_index, false);

    tasks[new_task_index].forked_pid = 0;
    tasks[new_task_index].is_dead = false;

    tasks[new_task_index].parent = tasks[index].pid;
    tasks[new_task_index].wait_pid = -1;

    abort();
    
    // for (uint16_t i = 0; i < 768; i++)
    // {
    //     uint32_t old_pde = read_physical_address_4b(tasks[index].cr3 + 4 * i);
    //     if (!(old_pde & 1)) continue;
    //     uint32_t new_pde = read_physical_address_4b(tasks[new_task_index].cr3 + 4 * i);
    //     physical_address_t old_pt_address = old_pde & 0xfffff000;
    //     physical_address_t new_pt_address = new_pde & 0xfffff000;
    //     if (!(new_pde & 1))
    //     {
    //         new_pt_address = pfa_allocate_physical_page();
    //         physical_init_page_table(new_pt_address);
    //         write_physical_address_4b(tasks[new_task_index].cr3 + 4 * i, new_pt_address | (old_pde & 0xfff));
    //     }
    //     // LOG(TRACE, "%u : old_pt_address : 0x%lx", i, old_pt_address);
    //     // LOG(TRACE, "%u : new_pt_address : 0x%lx", i, new_pt_address);
    //     for (uint16_t j = (i == 0 ? 256 : 0); j < 1024; j++)
    //     {
    //         uint32_t old_pte = read_physical_address_4b(old_pt_address + 4 * j);
    //         physical_address_t old_page_address = old_pte & 0xfffff000;
    //         if (old_pte & 1)
    //         {
    //             uint32_t new_pte = read_physical_address_4b(new_pt_address + 4 * j);
    //             physical_address_t new_page_address = new_pte & 0xfffff000;
    //             if (!(new_pte & 1))
    //             {
    //                 new_page_address = pfa_allocate_physical_page();
    //                 write_physical_address_4b(new_pt_address + 4 * j, new_page_address | (old_pte & 0xfff));
    //             }
    //             // LOG(TRACE, "%u.%u : old_page_address : 0x%lx", i, j, old_page_address);
    //             // LOG(TRACE, "%u.%u : new_page_address : 0x%lx", i, j, new_page_address);
    //             copy_page(old_page_address, new_page_address);
    //         }
    //     }
    // }
}

void cleanup_tasks()
{
    if (current_task_index != 0) return;

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
        if (!tasks[i].is_dead) continue;
        thread_t* parent = find_task_by_pid(tasks[i].parent);
        if (parent)
        {
            if (parent->wait_pid != -1)
            {
                if (parent->wait_pid == 0 || absint(parent->wait_pid) == tasks[i].pid)
                {
                    tasks[i].to_reap = true;
                    parent->wait_pid = -1;
                    parent->wstatus = tasks[i].return_value;
                }
            }
        }
        else
            tasks[i].to_reap = true;
    }

    for (uint16_t i = 0; i < task_count; i++)
    {
        if (i == current_task_index) continue;
        if (tasks[i].to_reap)
        {
            task_kill(i);
            i--;
            continue;
        }
    }
}