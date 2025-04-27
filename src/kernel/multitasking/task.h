#pragma once

struct interrupt_registers;

// #define KERNEL_STACK_SIZE 4096 // (2 * sizeof(struct interrupt_registers) + 24)  // 4 bytes for the pointer to the stack

struct task
{
    char* name;
    struct privilege_switch_interrupt_registers* registers_ptr;
    struct privilege_switch_interrupt_registers registers_data;
    physical_address_t stack_phys;
    physical_address_t kernel_stack_phys;
    utf32_buffer_t input_buffer;
    uint8_t ring;
    pid_t pid;
    bool system_task, kernel_thread;    // system_task: cause kernel panics ////; kernel_thread: dont allocate a vas
    bool reading_stdin, was_reading_stdin;
    physical_address_t page_directory_phys;
};

#define TASK_STACK_BOTTOM_ADDRESS           (0xc0000000 - 0x1000)
#define TASK_STACK_TOP_ADDRESS              (TASK_STACK_BOTTOM_ADDRESS + 0x1000)
#define TASK_KERNEL_STACK_BOTTOM_ADDRESS    (0xc0000000 - 2 * 0x1000)
#define TASK_KERNEL_STACK_TOP_ADDRESS       (TASK_KERNEL_STACK_BOTTOM_ADDRESS + 0x1000)

#define MAX_TASKS 8192

struct task tasks[MAX_TASKS];    // TODO : Implement a dynamic array
uint16_t task_count;

#define TASK_SWITCH_DELAY 30 // ms

uint8_t multitasking_counter = 0;

uint8_t page_tmp[4096];

uint16_t current_task_index = 0;
bool multitasking_enabled = false;
volatile bool first_task_switch = true;
uint64_t current_pid;

uint16_t zombie_task_index;

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

#define task_write_register_data(task_ptr, register, data)  write_physical_address_4b((physical_address_t)((uint32_t)(task_ptr)->registers_ptr) + (task_ptr)->stack_phys - TASK_STACK_BOTTOM_ADDRESS + offsetof(struct privilege_switch_interrupt_registers, register), data);
// ~~ Caller's responsability to check whether or not the task has the register actually pushed on the stack
void task_write_at_address_1b(struct task* _task, uint32_t address, uint8_t value);

void task_load_from_initrd(struct task* _task, char* path, uint8_t ring);
void task_destroy(struct task* _task);
void task_virtual_address_space_destroy(struct task* _task);
void task_virtual_address_space_create_page_table(struct task* _task, uint16_t index);
void task_virtual_address_space_remove_page_table(struct task* _task, uint16_t index);
physical_address_t task_virtual_address_space_create_page(struct task* _task, uint16_t pd_index, uint16_t pt_index, uint8_t user_supervisor, uint8_t read_write);
void task_create_virtual_address_space(struct task* _task);
void switch_task(struct privilege_switch_interrupt_registers** registers, bool* flush_tlb, uint32_t* iret_cr3);
void multasking_init();
void multitasking_start();
void multasking_add_task_from_initrd(char* path, uint8_t ring, bool system);  // TODO: Implement a vfs
void task_kill(uint16_t index);

void idle_main();