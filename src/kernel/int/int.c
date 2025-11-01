#pragma once

// #include "../multitasking/vas.h"
// #include "../multitasking/task.c"
// #include "../multitasking/syscall.h"
#include "int.h"
#include "irq.h"

#include "kernel_panic.h"

void handle_syscall(interrupt_registers_t* registers)
{
    ;
}

void switch_task()
{
    ;
}

#define return_from_isr() return

void interrupt_handler(interrupt_registers_t* registers)
{
    // LOG(DEBUG, "Interrupt number : %u", registers->interrupt_number);

    if (registers->interrupt_number < 32)            // Fault
    {
        LOG(WARNING, "Fault : Exception number : %u ; Error : %s ; Error code = 0x%x ; cr2 = 0x%x ; cr3 = 0x%x", registers->interrupt_number, get_error_message(registers->interrupt_number, registers->error_code), registers->error_code, registers->cr2, registers->cr3);
        
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

    if (registers->interrupt_number < 32 + 16)  // IRQ
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
    if (registers->interrupt_number == 0xf0)  // System call
    {
        if (!multitasking_enabled || first_task_switch) return_from_isr();

        handle_syscall(registers);
    }

    // * APIC interrupt

    // LOG(DEBUG, "Interrupt 0x%x from APIC", registers->interrupt_number);

    switch (registers->interrupt_number)
    {
    case 0x80:  // * APIC Timer
    {
        uint64_t increment = precise_time_to_milliseconds(GLOBAL_TIMER_INCREMENT);
        global_timer += GLOBAL_TIMER_INCREMENT;
        system_thousands += increment;
        resolve_time();

        if (system_thousands - increment < 500 && system_thousands >= 500)
        {
            tty_cursor_blink ^= true;
            if (tty_cursor_blink)
            {
                tty_render_cursor(tty_cursor);
            }
            else
            {
                uint16_t data = tty_data[tty_cursor];
                tty_render_character(tty_cursor, data, data >> 8);
            }
        }
        // printf("APIC Timer interrupt\n");
        break;
    }

    default:    // * Spurious interrupt (probably)
        return_from_isr();
    }

    lapic_send_eoi();

    return_from_isr();
}
