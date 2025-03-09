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
    fprintf(stdout, "Kernel panic\n\n\t");

    tty_color = (FG_WHITE | BG_BLUE);

    fprintf(stdout, "Exception number: %u\n\n\t", params->interrupt_number);
    fprintf(stdout, "Error:       %s\n\t", errorString[params->interrupt_number]);
    fprintf(stdout, "Error code:  0x%x\n\n\t", params->error_code);

    fprintf(stdout, "cr2:  0x%x\n", params->cr2);

    // LOG(ERROR, "Kernel panic : Exception number : %u ; Error : %s ; Error code = 0x%x", params->interrupt_number, errorString[params->interrupt_number], params->error_code);

    halt();
}

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

        // if (irqNumber != 0)
        //     LOG(INFO, "IRQ %u", irqNumber);

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

        case 1:
            handle_irq_1();
            break;
        case 12:
            handle_irq_12();
            break;

        default:
            break;
        }

        pic_send_eoi(irqNumber);
        return_from_isr();
    }
    else if (params->interrupt_number == 0xff)
    {
        // LOG(DEBUG, "Task \"%s\" (pid = %lu) sent system call %u", tasks[current_task_index].name, tasks[current_task_index].pid, params->eax);
        uint16_t old_index;
        switch (params->eax)
        {
        case 0:     // exit
            if (multitasking_enabled)
            {
                LOG(INFO, "Task \"%s\" (pid = %lu) exited with return code %d", tasks[current_task_index].name, tasks[current_task_index].pid, params->ebx);
                old_index = current_task_index;
                switch_task(&params);
                task_kill(old_index);
            }
            break;
        case 1:     // fputc
            if (params->ecx == (uint32_t)stdout || params->ecx == (uint32_t)stderr)
                putchar(params->ebx);
            else
                LOG(WARNING, "Unsupported file stream");
            break;
        case 2:     // time
            params->eax = ktime(NULL);
            break;
        case 3:     // getpid
            params->eax = tasks[current_task_index].pid >> 32;
            params->ebx = (uint32_t)tasks[current_task_index].pid;
            break;
        case 4:     // fork
            if (true) //(task_count >= MAX_TASKS)
            {
                params->eax = 0xffffffff;
                params->ebx = 0xffffffff;   // -1
            }
            else
            {
                // task_count++;
                // *(tasks[task_count - 1].registers) = params;
                // tasks[task_count - 1].registers->eax = tasks[task_count - 1].registers->ebx = 0;  // 0 to child
                // tasks[task_count - 1].pid = current_pid++;

                // params->eax = tasks[task_count - 1].pid >> 32;
                // params->ebx = (uint32_t)tasks[task_count - 1].pid;

                // tasks[task_count - 1].name = tasks[current_task_index].name;
                // tasks[task_count - 1].ring = tasks[current_task_index].ring;
                // if (tasks[current_task_index].kernel_stack)
                // {
                //     tasks[task_count - 1].kernel_stack = pfa_allocate_page();
                //     memcpy(tasks[task_count - 1].kernel_stack, tasks[current_task_index].kernel_stack, 4096);
                // }
                // if (tasks[current_task_index].stack)
                // {
                //     tasks[task_count - 1].stack = pfa_allocate_page();
                //     memcpy(tasks[task_count - 1].stack, tasks[current_task_index].stack, 4096);
                // }
                
                // !! TODO : Make the stack be mapped at a fixed place in memory so you can fork correctly
            }
            break;
        default:
            if (multitasking_enabled)
            {
                LOG(ERROR, "Undefined system call (0x%x)", params->eax);
                old_index = current_task_index;
                switch_task(&params);
                task_kill(old_index);
            }
        }            
    }

    return_from_isr();
}