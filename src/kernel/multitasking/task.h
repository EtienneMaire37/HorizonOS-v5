#pragma once

typedef struct task
{
    char* name;

    uint32_t esp, esp0, cr3;

    utf32_buffer_t input_buffer;
    bool reading_stdin, was_reading_stdin;

    uint8_t ring;
    pid_t pid;
    bool system_task;    // system_task: cause kernel panics

    fpu_state_t fpu_state;

    uint8_t stored_cpu_ticks, current_cpu_ticks;
} thread_t;

uint8_t global_cpu_ticks = 0;

const int task_esp_offset = offsetof(thread_t, esp);
const int task_esp0_offset = offsetof(thread_t, esp0);
const int task_cr3_offset = offsetof(thread_t, cr3);

#define MAX_TASKS 1024

thread_t tasks[MAX_TASKS];    // TODO : Implement a dynamic array
uint16_t task_count;

#define TASK_SWITCH_DELAY 40 // ms
#define TASK_SWITCHES_PER_SECOND (1000 / TASK_SWITCH_DELAY)

uint8_t multitasking_counter = 0;

uint16_t current_task_index = 0;
bool multitasking_enabled = false;
volatile bool first_task_switch = true;
uint64_t current_pid;

uint16_t zombie_task_index;

extern void iret_instruction();

extern void __attribute__((cdecl)) context_switch(thread_t* old_tcb, thread_t* next_tcb);
void full_context_switch(uint16_t next_task_index)
{
    int last_index = current_task_index;
    current_task_index = next_task_index;
    context_switch(&tasks[last_index], &tasks[current_task_index]);
}

bool task_is_blocked(uint16_t index)
{
    if (zombie_task_index != 0 && zombie_task_index == index) return true;
    if (tasks[index].reading_stdin) return true;
    return false;
}

uint16_t find_next_task_index() 
{
    uint16_t index = current_task_index;
    do 
    {
        index = (index + 1) % task_count;
    }
    while (task_is_blocked(index)); // Skip blocked tasks
    return index;
}

// #define task_write_register_data(task_ptr, register, data)  write_physical_address_4b((physical_address_t)((uint32_t)(task_ptr)->registers_ptr) + (task_ptr)->stack_phys - TASK_STACK_BOTTOM_ADDRESS + offsetof(struct privilege_switch_interrupt_registers, register), data);
// ~~ Caller's responsability to check whether or not the task has the register actually pushed on the stack
void task_write_at_address_1b(thread_t* _task, uint32_t address, uint8_t value);

void task_load_from_initrd(thread_t* _task, char* path, uint8_t ring);
void task_destroy(thread_t* _task);
void task_virtual_address_space_destroy(thread_t* _task);
void task_virtual_address_space_create_page_table(thread_t* _task, uint16_t index);
void task_virtual_address_space_remove_page_table(thread_t* _task, uint16_t index);
physical_address_t task_virtual_address_space_create_page(thread_t* _task, uint16_t pd_index, uint16_t pt_index, uint8_t user_supervisor, uint8_t read_write);
void task_create_virtual_address_space(thread_t* _task);
void switch_task(struct privilege_switch_interrupt_registers** registers);
void multitasking_init();
void multitasking_start();
void multitasking_add_task_from_initrd(char* path, uint8_t ring, bool system);  // TODO: Implement a vfs
void task_kill(uint16_t index);
void multitasking_add_idle_task();

void idle_main();