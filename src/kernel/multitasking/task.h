#pragma once

struct interrupt_registers;

// #define KERNEL_STACK_SIZE 4096 // (2 * sizeof(struct interrupt_registers) + 24)  // 4 bytes for the pointer to the stack

struct task
{
    // char name[32];
    char* name;
    struct interrupt_registers* registers;
    // uint8_t kernel_stack[KERNEL_STACK_SIZE];
    uint8_t* kernel_stack;
    // struct task* next_task;
    // struct task* previous_task;
    uint8_t* stack;
    uint8_t ring;
    uint64_t pid;
    struct page_directory_entry_4kb* page_directory;
};

struct task tasks[4096];    // TODO : Implement a dynamic array
uint16_t task_count;

#define TASK_SWITCH_DELAY 30 // ms

uint8_t multitasking_counter = 0;

// struct task* current_task;
uint16_t current_task_index = 0;
bool multitasking_enabled = false;
volatile bool first_task_switch = true;
uint64_t current_pid;

// void task_init(struct task* _task, uint32_t eip, char* name);
void task_load_from_initrd(struct task* _task, char* path, uint8_t ring);
void task_destroy(struct task* _task);
void task_virtual_address_space_destroy(struct task* _task);
void task_virtual_address_space_create_page_table(struct task* _task, uint16_t index);
void task_virtual_address_space_remove_page_table(struct task* _task, uint16_t index);
void task_virtual_address_space_create_page(struct task* _task, uint16_t pd_index, uint16_t pt_index, uint8_t user_supervisor, uint8_t read_write);
void task_create_virtual_address_space(struct task* _task);
void switch_task(struct interrupt_registers** registers);
void multasking_init();
void multitasking_start();
void multasking_add_task_from_initrd(char* path, uint8_t ring);  // TODO: Implement a vfs