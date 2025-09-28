#pragma once

#include "../multitasking/vas.h"
#include "../multitasking/task.c"
#include "../multitasking/syscall.h"

#include "kernel_panic.h"

#define return_from_isr() do { current_phys_mem_page = old_phys_mem_page; return; } while (0)

#define USE_IVLPG

void interrupt_handler(interrupt_registers_t* registers)
{
    uint32_t old_phys_mem_page = current_phys_mem_page;
    current_phys_mem_page = 0xffffffff;

    if (registers->interrupt_number < 32)            // Fault
    {
        LOG(WARNING, "Fault : Exception number : %u ; Error : %s ; Error code = 0x%x ; cr2 = 0x%x ; cr3 = 0x%x", registers->interrupt_number, get_error_message(registers->interrupt_number, registers->error_code), registers->error_code, registers->cr2, registers->cr3);

        if (registers->interrupt_number == 6 && *((uint16_t*)registers->eip) == 0xa20f)  // Invalid Opcode + CPUID // ~ Assumes no instruction prefix // !! Also assumes that eip does not cross a non present page boundary
        {
            has_cpuid = false;
            return_from_isr();
        }
        
        if (tasks[current_task_index].system_task || task_count == 1 || !multitasking_enabled || registers->interrupt_number == 8 || registers->interrupt_number == 18)
        // System task or last task or multitasking not enabled or Double Fault or Machine Check
        {
            disable_interrupts();
            kernel_panic((interrupt_registers_t*)registers);
        }
        else
        {
            tasks[current_task_index].is_dead = true;
            switch_task(&registers);
            current_phys_mem_page = 0xffffffff;
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

        bool ts = false;

        switch (irq_number)
        {
        case 0:
            handle_irq_0(&ts);
            break;

        case 1:
            handle_irq_1();
            break;
        case 12:
            handle_irq_12();
            break;

        default:
            break;
        }

        pic_send_eoi(irq_number);

        if (ts) 
        {
            switch_task(&registers);
            current_phys_mem_page = 0xffffffff;
        }
        return_from_isr();
    }
    if (registers->interrupt_number == 0xf0)  // System call
    {
        if (!multitasking_enabled || first_task_switch) return_from_isr();

        handle_syscall(registers);
    }

    return_from_isr();
}
