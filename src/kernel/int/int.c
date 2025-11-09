#pragma once

#include "../multitasking/vas.h"
// #include "../multitasking/task.c"
// #include "../multitasking/syscall.h"
#include "int.h"
#include "irq.h"

#include "kernel_panic.h"

void handle_syscall(interrupt_registers_t* registers)
{
    ;
}

#define return_from_isr() return

void interrupt_handler(interrupt_registers_t* registers)
{
    if (registers->interrupt_number < 32)       // * Fault
    {
        LOG(WARNING, "Fault : Exception number : %llu ; Error : %s ; Error code = 0x%llx ; cr2 = 0x%llx ; cr3 = 0x%llx ; rip = 0x%llx", registers->interrupt_number, get_error_message(registers->interrupt_number, registers->error_code), 
        registers->error_code, registers->cr2, registers->cr3, registers->rip);
        
        if (tasks[current_task_index].system_task || task_count == 1 || !multitasking_enabled || registers->interrupt_number == 8 || registers->interrupt_number == 18)
        // System task or last task or multitasking not enabled or Double Fault or Machine Check
        {
            disable_interrupts();
            kernel_panic((interrupt_registers_t*)registers);
        }
        else
        {
            if (registers->interrupt_number == 14)  // * Page fault
                printf("Segmentation fault\n");
            tasks[current_task_index].is_dead = true;
            tasks[current_task_index].return_value = 0x80000000;
            switch_task();
        }

        return_from_isr();
    }

    if (registers->interrupt_number < 32 + 16)  // * IRQ
    {
        uint8_t irq_number = registers->interrupt_number - 32;

        if (irq_number == 7 && !(pic_get_isr() >> 7))
            return_from_isr();
        if (irq_number == 15 && !(pic_get_isr() >> 15))
        {
            outb(PIC1_CMD, PIC_EOI);
	        io_wait();
            return_from_isr();
        }

        pic_send_eoi(irq_number);

        return_from_isr();
    }
    if (registers->interrupt_number == 0xf0)    // * System call
    // TODO: Switch to using the syscall/sysret instructions
    {
        if (!multitasking_enabled || first_task_switch) return_from_isr();

        // LOG(DEBUG, "Received syscall!!");

        handle_syscall(registers);

        return_from_isr();
    }

    // * APIC interrupt (probably)

    handle_apic_irq(registers);

    return_from_isr();
}
