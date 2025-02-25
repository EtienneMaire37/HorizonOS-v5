#pragma once

struct interrupt_registers;

#define KERNEL_STACK_SIZE (sizeof(struct interrupt_registers) + 4)  // 4 bytes for the pointer to the stack

struct task
{
    char name[32];
    struct interrupt_registers* registers;
    uint8_t kernel_stack[KERNEL_STACK_SIZE];
    struct task* next_task;
    struct task* previous_task;
    // uint8_t stack[4096];
    uint8_t* stack;
    uint8_t ring;
    struct page_directory_entry_4kb* page_directory;
};

#define TASK_SWITCH_DELAY 30 // ms

uint8_t multitasking_counter = 0;

struct task* current_task;
bool multitasking_enabled = false;
volatile bool first_task_switch = true;

void task_init(struct task* _task, uint32_t eip, char* name);
void task_load_from_initrd(struct task* _task, char* name, uint8_t ring);
void task_destroy(struct task* _task);
void task_virtual_address_space_destroy(struct task* _task);
void task_virtual_address_space_create_page_table(struct task* _task, uint16_t index);
void task_virtual_address_space_remove_page_table(struct task* _task, uint16_t index);
void task_virtual_address_space_create_page(struct task* _task, uint16_t pd_index, uint16_t pt_index, uint8_t user_supervisor, uint8_t read_write);
void task_create_virtual_address_space(struct task* _task);
void switch_task(struct interrupt_registers** registers);