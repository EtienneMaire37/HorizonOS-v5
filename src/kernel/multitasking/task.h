#pragma once

#include "../io/keyboard.h"

#define THREAD_NAME_MAX 64

typedef struct thread
{
    char name[THREAD_NAME_MAX];

    uint64_t rsp, cr3;

    utf32_buffer_t input_buffer;
    bool reading_stdin, is_dead;

    uint32_t return_value;

    pid_t wait_pid;
    int wstatus;

    bool to_reap;

    pid_t parent;

    pid_t forked_pid;

    uint8_t ring;
    pid_t pid;
    bool system_task;    // system_task: cause kernel panics

    fpu_state_t fpu_state;

    uint16_t stored_cpu_ticks, current_cpu_ticks;   // * In milliseconds

    file_table_index_t file_table[OPEN_MAX];
} thread_t;

uint8_t global_cpu_ticks = 0;

const int task_rsp_offset = offsetof(thread_t, rsp);
const int task_cr3_offset = offsetof(thread_t, cr3);

#define TASK_STACK_PAGES        0x100       // 1MB
#define TASK_KERNEL_STACK_PAGES 32          // 128KB

#define TASK_STACK_TOP_ADDRESS              0x800000000000 // 0x7fffffffffff
#define TASK_STACK_BOTTOM_ADDRESS           (TASK_STACK_TOP_ADDRESS - 0x1000 * TASK_STACK_PAGES)
#define TASK_KERNEL_STACK_TOP_ADDRESS       TASK_STACK_BOTTOM_ADDRESS
#define TASK_KERNEL_STACK_BOTTOM_ADDRESS    (TASK_KERNEL_STACK_TOP_ADDRESS - 0x1000 * TASK_KERNEL_STACK_PAGES)

#define MAX_TASKS 256

thread_t tasks[MAX_TASKS];    // TODO : Implement a dynamic array
uint16_t task_count;

#define TASK_SWITCH_DELAY 40 // ms
#define TASK_SWITCHES_PER_SECOND (1000 / TASK_SWITCH_DELAY)

uint16_t current_task_index = 0;
bool multitasking_enabled = false;
volatile bool first_task_switch = true;
uint64_t current_pid;

extern void iretq_instruction();
void task_kill(uint16_t index);

void pic_enable();
void pic_disable();

void lock_task_queue()
{
    // disable_interrupts();
    pic_disable();
}

void unlock_task_queue()
{
    // enable_interrupts();
    pic_enable();
}

int vfs_allocate_thread_file(int index)
{
    for (int i = 3; i < OPEN_MAX; i++)
    {
        if (tasks[index].file_table[i] == invalid_fd)
            return i;
    }
    return -1;
}

extern void context_switch(thread_t* old_tcb, thread_t* next_tcb, uint64_t ds, uint8_t* old_fpu_state, uint8_t* next_fpu_state);
extern void fork_context_switch(thread_t* next_tcb);
void full_context_switch(uint16_t next_task_index)
{
    int last_index = current_task_index;
    current_task_index = next_task_index;
    TSS.rsp0 = TASK_KERNEL_STACK_TOP_ADDRESS;
    uint8_t* old_fpu_state = (uint8_t*)(((uintptr_t)&tasks[last_index].fpu_state.data[0] & 0xfffffff0) + 16);
    uint8_t* new_fpu_state = (uint8_t*)(((uintptr_t)&tasks[current_task_index].fpu_state.data[0] & 0xfffffff0) + 16);
    context_switch(&tasks[last_index], &tasks[current_task_index], tasks[current_task_index].ring == 0 ? KERNEL_DATA_SEGMENT : USER_DATA_SEGMENT,
    old_fpu_state, new_fpu_state);
}

bool task_is_blocked(uint16_t index)
{
    if (tasks[index].is_dead) return true;
    if (tasks[index].reading_stdin) return true;
    if (tasks[index].forked_pid) return true;
    if (tasks[index].wait_pid != -1) return true;
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

static inline void task_write_at_address_1b(thread_t* task, uint64_t address, uint8_t value);

void task_destroy(thread_t* task);
void switch_task();
void multitasking_init();
void multitasking_start();
void task_kill(uint16_t index);
void multitasking_add_idle_task();

void task_stack_push(thread_t*, uint64_t);

void cleanup_tasks();

void idle_main();