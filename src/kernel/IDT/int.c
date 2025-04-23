#pragma once

void kernel_panic(struct privilege_switch_interrupt_registers* registers)
{
    disable_interrupts();

    tty_color = BG_BLUE;
    tty_clear_screen(' ');

    tty_cursor = 4 + 2 * 80;
    tty_update_cursor();
    tty_hide_cursor();

    tty_color = (FG_RED | BG_BLUE);
    printf("Kernel panic\n\n\t");

    tty_color = (FG_WHITE | BG_BLUE);

    printf("Exception number: %u\n\n\t", registers->interrupt_number);
    printf("Error:       %s\n\t", get_error_message(registers->interrupt_number, registers->error_code));
    printf("Error code:  0x%x\n\n\t", registers->error_code);

    if (registers->interrupt_number == 14)
    {
        printf("cr2:  0x%x (pde %u pte %u offset 0x%x)\n\t", registers->cr2, registers->cr2 >> 22, (registers->cr2 >> 12) & 0x3ff, registers->cr2 & 0xfff);
        printf("cr3:  0x%x\n\t", registers->cr3);

        uint32_t pde = read_physical_address_4b(current_cr3 + 4 * (registers->cr2 >> 22));
        
        LOG(DEBUG, "Page directory entry : 0x%x", pde);

        if (pde & 1)
        {
            LOG(DEBUG, "Page table entry : 0x%x", read_physical_address_4b((pde & 0xfffff000) + 4 * ((registers->cr2 >> 12) & 0x3ff)));
        }
    }

    halt();
}

#define return_from_isr() { current_cr3 = iret_cr3; current_phys_mem_page = old_phys_mem_page; return flush_tlb ? iret_cr3 : 0; }

uint32_t __attribute__((cdecl)) interrupt_handler(struct privilege_switch_interrupt_registers* registers)
{
    current_cr3 = registers->cr3; 
    uint32_t old_phys_mem_page = current_phys_mem_page;
    current_phys_mem_page = 0xffffffff;
    flush_tlb = false;

    iret_cr3 = registers->cr3;
    user_mode_switch = multitasking_enabled ? (tasks[current_task_index].ring != 0 ? 1 : 0) : 0;

    if (registers->interrupt_number < 32)            // Fault
    {
        LOG(ERROR, "Fault : Exception number : %u ; Error : %s ; Error code = 0x%x ; cr2 = 0x%x ; cr3 = 0x%x", registers->interrupt_number, get_error_message(registers->interrupt_number, registers->error_code), registers->error_code, registers->cr2, registers->cr3);

        if (tasks[current_task_index].system_task || task_count == 1 || !multitasking_enabled || registers->interrupt_number == 8 || registers->interrupt_number == 18)
        // System task or last task or multitasking not enabled or Double Fault or Machine Check
            kernel_panic(registers);
        else
        {
            if (zombie_task_index != 0 && zombie_task_index != current_task_index)
            {
                task_kill(zombie_task_index);
                zombie_task_index = 0;
            }

            uint16_t old_index = current_task_index;
            switch_task(&registers);
            task_kill(old_index);
        }

        return_from_isr();
    }

    if (zombie_task_index != 0 && zombie_task_index != current_task_index) // IRQ or syscall
    {
        task_kill(zombie_task_index);
        zombie_task_index = 0;
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

        switch (irq_number)
        {
        case 0:
            handle_irq_0(&registers);
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
        return_from_isr();
    }
    if (registers->interrupt_number == 0xff)  // System call
    {
        // LOG(DEBUG, "Task \"%s\" (pid = %lu) sent system call %u", tasks[current_task_index].name, tasks[current_task_index].pid, registers->eax);
        uint16_t old_index;
        switch (registers->eax)
        {
        case 0:     // exit
            if (multitasking_enabled)
            {
                LOG(INFO, "Task \"%s\" (pid = %lu) exited with return code %d", tasks[current_task_index].name, tasks[current_task_index].pid, registers->ebx);
                if (zombie_task_index != 0)
                {
                    LOG(CRITICAL, "Tried to kill several tasks at once");
                    abort();
                }
                zombie_task_index = current_task_index;
            }
            break;
        case 1:     // write
            if (registers->ebx > 2)
            {
                registers->eax = 0xffffffff;   // -1
                registers->ebx = EBADF;
            }
            else
            {
                registers->eax = write(registers->ebx, (char*)registers->ecx, registers->edx);
                registers->ebx = errno;
            }
            break;
        case 2:     // time
            registers->eax = ktime(NULL);
            break;
        case 3:     // getpid
            registers->eax = tasks[current_task_index].pid >> 32;
            registers->ebx = (uint32_t)tasks[current_task_index].pid;
            break;
        case 4:     // fork
            LOG(DEBUG, "Forking task \"%s\" (pid = %lu)", tasks[current_task_index].name, tasks[current_task_index].pid);

            if (task_count >= MAX_TASKS)
            {
                registers->eax = 0xffffffff;
                registers->ebx = 0xffffffff;   // -1
            }
            else
            {
                task_count++;

                tasks[current_task_index].registers_data = *registers;
                tasks[current_task_index].registers_ptr = registers;

                tasks[task_count - 1].name = tasks[current_task_index].name;
                tasks[task_count - 1].ring = tasks[current_task_index].ring;
                tasks[task_count - 1].pid = current_pid++;
                tasks[task_count - 1].system_task = tasks[current_task_index].system_task;
                tasks[task_count - 1].kernel_thread = tasks[current_task_index].kernel_thread;

                if (tasks[current_task_index].stack_phys)
                    tasks[task_count - 1].stack_phys = pfa_allocate_physical_page();
                if (tasks[current_task_index].kernel_stack_phys)
                    tasks[task_count - 1].kernel_stack_phys = pfa_allocate_physical_page();

                tasks[task_count - 1].registers_ptr = tasks[current_task_index].registers_ptr;
                tasks[task_count - 1].registers_data = tasks[current_task_index].registers_data;

                task_create_virtual_address_space(&tasks[task_count - 1]);

                if (tasks[task_count - 1].kernel_stack_phys)
                    copy_page(tasks[current_task_index].kernel_stack_phys, tasks[task_count - 1].kernel_stack_phys);
                if (tasks[task_count - 1].stack_phys)
                    copy_page(tasks[current_task_index].stack_phys, tasks[task_count - 1].stack_phys);

                tasks[task_count - 1].registers_data.eax = tasks[task_count - 1].registers_data.ebx = 0;
                tasks[current_task_index].registers_data.eax = tasks[task_count - 1].pid >> 32;
                tasks[current_task_index].registers_data.ebx = tasks[task_count - 1].pid;

                for (uint16_t i = 0; i < 768; i++)
                {
                    uint32_t old_pde = read_physical_address_4b(tasks[current_task_index].page_directory_phys + 4 * i);
                    for (uint16_t j = (i == 0 ? 256 : 0); j < ((i == 767) ? 1021 : 1024); j++)
                    {
                        if (old_pde & 1)
                        {
                            physical_address_t pt_old_phys = old_pde & 0xfffff000;
                            uint32_t old_pte = read_physical_address_4b(pt_old_phys + 4 * j);
                            physical_address_t mapping_phys = old_pte & 0xfffff000;
                            if (old_pte & 1)
                            {
                                // LOG(TRACE, "Copying 0x%x-0x%x", 4096 * (j + i * 1024), 4095 + 4096 * (j + i * 1024));

                                physical_address_t page_address = task_virtual_address_space_create_page(&tasks[task_count - 1], i, j, PAGING_USER_LEVEL, true);

                                copy_page(mapping_phys, page_address);
                            }
                        }
                    }
                }

                for (uint16_t i = 0; i < (tasks[task_count - 1].ring != 0 ? sizeof(struct privilege_switch_interrupt_registers) : sizeof(struct interrupt_registers)); i++)
                    write_physical_address_1b((physical_address_t)((uint32_t)tasks[task_count - 1].registers_ptr) + tasks[task_count - 1].stack_phys - TASK_STACK_BOTTOM_ADDRESS + i,
                        ((uint8_t*)&tasks[task_count - 1].registers_data)[i]);

                if (tasks[current_task_index].ring != 0)
                {
                    registers->esp = tasks[current_task_index].registers_data.esp;
                    registers->ss = tasks[current_task_index].registers_data.ss;
                }

                *(struct interrupt_registers*)registers = *(struct interrupt_registers*)&tasks[current_task_index].registers_data;

                switch_task(&registers);
            } 
            break;

        default:
            if (multitasking_enabled)
            {
                LOG(ERROR, "Undefined system call (0x%x)", registers->eax);
                old_index = current_task_index;
                switch_task(&registers);
                task_kill(old_index);
            }
        }            
    }

    return_from_isr();
}
