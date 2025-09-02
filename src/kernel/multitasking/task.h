#pragma once

typedef struct task
{
    char* name;
    physical_address_t stack_phys;
    physical_address_t kernel_stack_phys;
    utf32_buffer_t input_buffer;
    bool reading_stdin, was_reading_stdin;

    uint32_t esp, esp0, cr3;

    uint8_t ring;
    pid_t pid;
    bool system_task;    // system_task: cause kernel panics

    fpu_state_t fpu_state;
} task_t;

int task_esp_offset = offsetof(task_t, esp);
int task_esp0_offset = offsetof(task_t, esp0);
int task_cr3_offset = offsetof(task_t, cr3);

#define TASK_STACK_BOTTOM_ADDRESS           (0xc0000000 - 0x1000)
#define TASK_STACK_TOP_ADDRESS              (TASK_STACK_BOTTOM_ADDRESS + 0x1000)
#define TASK_KERNEL_STACK_BOTTOM_ADDRESS    (0xc0000000 - 2 * 0x1000)
#define TASK_KERNEL_STACK_TOP_ADDRESS       (TASK_KERNEL_STACK_BOTTOM_ADDRESS + 0x1000)

#define MAX_TASKS 256

task_t tasks[MAX_TASKS];    // TODO : Implement a dynamic array
uint16_t task_count;

#define TASK_SWITCH_DELAY 30 // ms

uint8_t multitasking_counter = 0;

uint16_t current_task_index = 0;
bool multitasking_enabled = false;
volatile bool first_task_switch = true;
uint64_t current_pid;

uint16_t zombie_task_index;

extern void __attribute__((cdecl)) context_switch(task_t* old_tcb, task_t* next_tcb);
void full_context_switch(uint16_t next_task_index)
{
    context_switch(&tasks[current_task_index], &tasks[next_task_index]);
    current_task_index = next_task_index;
    TSS.esp0 = tasks[current_task_index].esp0;
}

uint16_t find_next_task_index() 
{
    uint16_t index = current_task_index;
    do 
    {
        index = (index + 1) % task_count;
    }
    while (tasks[index].reading_stdin); // Skip blocked tasks
    return index;
}

// #define task_write_register_data(task_ptr, register, data)  write_physical_address_4b((physical_address_t)((uint32_t)(task_ptr)->registers_ptr) + (task_ptr)->stack_phys - TASK_STACK_BOTTOM_ADDRESS + offsetof(struct privilege_switch_interrupt_registers, register), data);
// ~~ Caller's responsability to check whether or not the task has the register actually pushed on the stack
void task_write_at_address_1b(task_t* _task, uint32_t address, uint8_t value);

void task_load_from_initrd(task_t* _task, char* path, uint8_t ring);
void task_destroy(task_t* _task);
void task_virtual_address_space_destroy(task_t* _task);
void task_virtual_address_space_create_page_table(task_t* _task, uint16_t index);
void task_virtual_address_space_remove_page_table(task_t* _task, uint16_t index);
physical_address_t task_virtual_address_space_create_page(task_t* _task, uint16_t pd_index, uint16_t pt_index, uint8_t user_supervisor, uint8_t read_write);
void task_create_virtual_address_space(task_t* _task);
void switch_task(struct privilege_switch_interrupt_registers** registers);
void multasking_init();
void multitasking_start();
void multasking_add_task_from_initrd(char* path, uint8_t ring, bool system);  // TODO: Implement a vfs
void task_kill(uint16_t index);

void idle_main();