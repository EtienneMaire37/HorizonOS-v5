#pragma once

typedef struct task
{
    char* name;

    volatile uint32_t esp, cr3;

    utf32_buffer_t input_buffer;
    bool reading_stdin, was_reading_stdin, is_dead;

    pid_t forked_pid;

    uint8_t ring;
    pid_t pid;
    bool system_task;    // system_task: cause kernel panics

    fpu_state_t fpu_state;

    uint16_t stored_cpu_ticks, current_cpu_ticks;   // * In milliseconds
} thread_t;

uint8_t global_cpu_ticks = 0;

const int task_esp_offset = offsetof(thread_t, esp);
// const int task_esp0_offset = offsetof(thread_t, esp0);
const int task_cr3_offset = offsetof(thread_t, cr3);

#define TASK_STACK_PAGES        0x100  // 1MB
#define TASK_KERNEL_STACK_PAGES 1   // 4KB

#define TASK_STACK_TOP_ADDRESS              0xc0000000
#define TASK_STACK_BOTTOM_ADDRESS           (TASK_STACK_TOP_ADDRESS - 0x1000 * TASK_STACK_PAGES)
#define TASK_KERNEL_STACK_TOP_ADDRESS       TASK_STACK_BOTTOM_ADDRESS
#define TASK_KERNEL_STACK_BOTTOM_ADDRESS    (TASK_KERNEL_STACK_TOP_ADDRESS - 0x1000)

const int kernel_stack_page_index = (TASK_KERNEL_STACK_BOTTOM_ADDRESS - (uint32_t)767 * 0x400000) / 0x1000;

const int stack_page_index_start = (TASK_STACK_BOTTOM_ADDRESS - (uint32_t)767 * 0x400000) / 0x1000;
const int stack_page_index_end = (TASK_STACK_TOP_ADDRESS - (uint32_t)767 * 0x400000) / 0x1000 - 1;

#define MAX_TASKS 1024

thread_t tasks[MAX_TASKS];    // TODO : Implement a dynamic array
uint16_t task_count;

#define TASK_SWITCH_DELAY 40 // ms
#define TASK_SWITCHES_PER_SECOND (1000 / TASK_SWITCH_DELAY)

uint16_t current_task_index = 0;
bool multitasking_enabled = false;
volatile bool first_task_switch = true;
uint64_t current_pid;

extern void iret_instruction();
void task_kill(uint16_t index);

void lock_task_queue()
{
    disable_interrupts();
}

void unlock_task_queue()
{
    enable_interrupts();
}

extern void __attribute__((cdecl)) context_switch(thread_t* old_tcb, thread_t* next_tcb, uint32_t ds);
extern void __attribute__((cdecl)) fork_context_switch(thread_t* next_tcb);
void full_context_switch(uint16_t next_task_index)
{
    int last_index = current_task_index;
    current_task_index = next_task_index;
    TSS.esp0 = TASK_KERNEL_STACK_TOP_ADDRESS;
    context_switch(&tasks[last_index], &tasks[current_task_index], tasks[current_task_index].ring == 0 ? KERNEL_DATA_SEGMENT : USER_DATA_SEGMENT);
}

bool task_is_blocked(uint16_t index)
{
    if (tasks[index].is_dead) return true;
    if (tasks[index].reading_stdin) return true;
    if (tasks[index].forked_pid) return true;
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

void task_write_at_address_1b(volatile thread_t* task, uint32_t address, uint8_t value);

void task_destroy(thread_t* task);
void switch_task(volatile struct interrupt_registers** registers);
void multitasking_init();
void multitasking_start();
void task_kill(uint16_t index);
void multitasking_add_idle_task();

void task_stack_push(volatile thread_t*, uint32_t);

void cleanup_tasks();

void idle_main();