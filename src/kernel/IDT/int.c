#pragma once

void kernel_panic(struct interrupt_registers* params)
{
    disable_interrupts();

    tty_color = BG_BLUE;
    tty_clear_screen(' ');

    tty_cursor = 4 + 2 * 80;
    tty_update_cursor();
    tty_hide_cursor();

    tty_color = (FG_LIGHTRED | BG_BLUE);
    kfprintf(kstdout, "Kernel panic\n\n\t");

    tty_color = (FG_WHITE | BG_BLUE);

    kfprintf(kstdout, "Exception number: %u\n\n\t", params->interrupt_number);
    kfprintf(kstdout, "Error:       %s\n\t", errorString[params->interrupt_number]);
    kfprintf(kstdout, "Error code:  0x%x\n\n\t", params->error_code);

    kfprintf(kstdout, "cr2:  0x%x\n", params->cr2);

    // LOG(ERROR, "Kernel panic : Exception number : %u ; Error : %s ; Error code = 0x%x", params->interrupt_number, errorString[params->interrupt_number], params->error_code);

    halt();
}

uint32_t _cr3;

// #define return_from_isr() { return task_data_segment(*currentTask); }
#define return_from_isr() { return params->cr3; }

uint32_t __attribute__((cdecl)) interrupt_handler(struct interrupt_registers* params)
{
    // if (multitasking_enabled)
    //     LOG(DEBUG, "Current registers : esp : 0x%x, 0x%x | eip : 0x%x", 
    //         params->esp, params->handled_esp, params->eip);

    // if (current_task->name[0] == '.')   // TASK A
    //     LOG(DEBUG, "0x100000 : 0x%x", *(uint32_t*)0x100000);

    if (params->interrupt_number < 32)            // Fault
    {
        LOG(ERROR, "Fault : Exception number : %u ; Error : %s ; Error code = 0x%x ; cr2 = 0x%x", params->interrupt_number, errorString[params->interrupt_number], params->error_code, params->cr2);

        if (tasks[current_task_index].system_task || task_count == 1 || !multitasking_enabled)
            kernel_panic(params);
        else
        {
            uint16_t old_index = current_task_index;
            switch_task(&params);
            task_kill(old_index);
        }

        return_from_isr();
    }
    else if (params->interrupt_number < 32 + 16)  // IRQ
    {
        uint8_t irqNumber = params->interrupt_number - 32;

        if (irqNumber == 7 && !(pic_get_isr() >> 7))
            return_from_isr();
        if (irqNumber == 15 && !(pic_get_isr() >> 15))
        {
            outb(PIC1_CMD, PIC_EOI);
	        io_wait();
            return_from_isr();
        }
        
        // LOG(INFO, "Interrupt %u handled", params->interrupt_number);

        switch (irqNumber)
        {
        case 0:
            handle_irq_0();
            if (time_initialized)
                system_increment_time();
            {
                if (multitasking_enabled)
                {
                    multitasking_counter--;
                    if (multitasking_counter == 0)
                    {
                        switch_task(&params);
                        multitasking_counter = TASK_SWITCH_DELAY / PIT_INCREMENT;
                    }
                    if (multitasking_counter == 0xff)
                        multitasking_counter = TASK_SWITCH_DELAY / PIT_INCREMENT;
                }
            }
            break;

        default:
            break;
        }

        pic_send_eoi(irqNumber);
        return_from_isr();
    }
    else if (params->interrupt_number == 0xff)
    {
        uint16_t old_index;
        switch (params->eax)
        {
        case 0:
            if (multitasking_enabled)
            {
                LOG(INFO, "Task \"%s\" (pid = %lu) exited with return code %d", tasks[current_task_index].name, tasks[current_task_index].pid, params->ebx);
                LOG(ERROR, "Undefined system call");
                old_index = current_task_index;
                switch_task(&params);
                task_kill(old_index);
            }
            break;
        case 1:
            if (params->ecx == (uint32_t)kstdout || params->ecx == (uint32_t)kstderr)
                kputchar(params->ebx);
            else
                LOG(WARNING, "Unsupported file stream");
            break;
        case 2:
            params->eax = ktime(NULL);
            break;
        default:
            if (multitasking_enabled)
            {
                LOG(ERROR, "Undefined system call");
                old_index = current_task_index;
                switch_task(&params);
                task_kill(old_index);
            }
        }            
    }

    return_from_isr();
}