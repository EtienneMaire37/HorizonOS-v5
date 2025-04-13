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
    printf("Kernel panic\n\n\t");

    tty_color = (FG_WHITE | BG_BLUE);

    printf("Exception number: %u\n\n\t", params->interrupt_number);
    printf("Error:       %s\n\t", get_error_message(params->interrupt_number, params->error_code));
    printf("Error code:  0x%x\n\n\t", params->error_code);
    printf("Segment error code:  %s|%s|0x%x\n\n\t", 
        params->error_code & 1 ? "External" : "Internal",
        seg_error_code[(params->error_code >> 1) & 0b11],
        (params->error_code >> 3) & 0b1111111111111);

    if (params->interrupt_number == 14)
    {
        printf("cr2:  0x%x (pde %u pte %u offset 0x%x)\n\t", params->cr2, params->cr2 >> 22, (params->cr2 >> 12) & 0x3ff, params->cr2 & 0xfff);
        printf("cr3:  0x%x\n\t", params->cr3);

        uint32_t pde = read_physical_address_4b(current_cr3 + 4 * (params->cr2 >> 22));
        
        LOG(DEBUG, "Page directory entry : 0x%x", pde);

        if (pde & 1)
        {
            LOG(DEBUG, "Page table entry : 0x%x", read_physical_address_4b((pde & 0xfffff000) + 4 * ((params->cr2 >> 12) & 0x3ff)));
        }
    }

    halt();
}

#define return_from_isr() { return params->cr3; }

uint32_t __attribute__((cdecl)) interrupt_handler(struct interrupt_registers* params)
{
    current_cr3 = params->cr3; 
    current_phys_mem_page = 0xffffffff;

    if (multitasking_enabled && !first_task_switch)
    {
        tasks[current_task_index].registers_data = *params;
        // LOG(TRACE, "Saved register data");
    }

    if (params->interrupt_number < 32)            // Fault
    {
        LOG(ERROR, "Fault : Exception number : %u ; Error : %s ; Error code = 0x%x ; cr2 = 0x%x ; cr3 = 0x%x", params->interrupt_number, get_error_message(params->interrupt_number, params->error_code), params->error_code, params->cr2, params->cr3);

        if (tasks[current_task_index].system_task || task_count == 1 || !multitasking_enabled || params->interrupt_number == 8 || params->interrupt_number == 18)
        // System task or last task or multitasking not enabled or Double Fault or Machine Check
            kernel_panic(params);
        else
        {
            if (zombie_task_index != 0 && zombie_task_index != current_task_index)
            {
                task_kill(zombie_task_index);
                zombie_task_index = 0;
            }

            uint16_t old_index = current_task_index;
            switch_task(&params);
            task_kill(old_index);
        }

        return_from_isr();
    }

    if (zombie_task_index != 0 && zombie_task_index != current_task_index) // IRQ or syscall
    {
        task_kill(zombie_task_index);
        zombie_task_index = 0;
    }

    if (params->interrupt_number < 32 + 16)  // IRQ
    {
        uint8_t irq_number = params->interrupt_number - 32;

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
            handle_irq_0(params);
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
    if (params->interrupt_number == 0xff)  // System call
    {
        // LOG(DEBUG, "Task \"%s\" (pid = %lu) sent system call %u", tasks[current_task_index].name, tasks[current_task_index].pid, params->eax);
        uint16_t old_index;
        switch (params->eax)
        {
        case 0:     // exit
            if (multitasking_enabled)
            {
                LOG(INFO, "Task \"%s\" (pid = %lu) exited with return code %d", tasks[current_task_index].name, tasks[current_task_index].pid, params->ebx);
                if (zombie_task_index != 0)
                {
                    LOG(CRITICAL, "Tried to kill several tasks at once");
                    abort();
                }
                zombie_task_index = current_task_index;
            }
            break;
        case 1:     // write
            if (params->ebx > 2)
            {
                params->eax = 0xffffffff;   // -1
                params->ebx = EBADF;
            }
            else
            {
                params->eax = write(params->ebx, (char*)params->ecx, params->edx);
                params->ebx = errno;
            }
            break;
        case 2:     // time
            params->eax = ktime(NULL);
            break;
        case 3:     // getpid
            params->eax = tasks[current_task_index].pid >> 32;
            params->ebx = (uint32_t)tasks[current_task_index].pid;
            break;
        case 4:     // fork
            LOG(DEBUG, "Forking task \"%s\" (pid = %lu)", tasks[current_task_index].name, tasks[current_task_index].pid);

            if (true) // (task_count >= MAX_TASKS)
            {
                params->eax = 0xffffffff;
                params->ebx = 0xffffffff;   // -1
            }
            else
            {
                task_count++;

                tasks[task_count - 1].name = tasks[current_task_index].name;
                tasks[task_count - 1].ring = tasks[current_task_index].ring;
                tasks[task_count - 1].pid = current_pid++;
                tasks[task_count - 1].system_task = tasks[current_task_index].system_task;
                tasks[task_count - 1].kernel_thread = tasks[current_task_index].kernel_thread;

                if (tasks[current_task_index].stack_phys)
                    tasks[task_count - 1].stack_phys = pfa_allocate_physical_page();
                if (tasks[current_task_index].kernel_stack_phys)
                    tasks[task_count - 1].kernel_stack_phys = pfa_allocate_physical_page();

                tasks[task_count - 1].registers_ptr = (struct interrupt_registers*)(TASK_STACK_TOP_ADDRESS - sizeof(struct interrupt_registers));

                tasks[task_count - 1].registers_data = tasks[current_task_index].registers_data;

                task_create_virtual_address_space(&tasks[task_count - 1]);

                if (tasks[task_count - 1].kernel_stack_phys)
                {
                    set_current_phys_mem_page(tasks[current_task_index].kernel_stack_phys >> 12);
                    memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
                    set_current_phys_mem_page(tasks[task_count - 1].kernel_stack_phys >> 12);
                    memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
                }
                if (tasks[task_count - 1].stack_phys)
                {
                    set_current_phys_mem_page(tasks[current_task_index].stack_phys >> 12);
                    memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
                    set_current_phys_mem_page(tasks[task_count - 1].stack_phys >> 12);
                    memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
                }

                tasks[task_count - 1].registers_data.eax = tasks[task_count - 1].registers_data.ebx = 0;
                tasks[current_task_index].registers_data.eax = tasks[task_count - 1].pid >> 32;
                tasks[current_task_index].registers_data.ebx = tasks[task_count - 1].pid;

                for (uint16_t i = 0; i < 768; i++)
                {
                    for (uint16_t j = (i == 0 ? 256 : 0); j < ((i == 767) ? 1021 : 1024); j++)
                    {
                        uint32_t old_pde = read_physical_address_4b(tasks[current_task_index].page_directory_phys + 4 * i);
                        if (old_pde & 1)
                        {
                            physical_address_t pt_old_phys = old_pde & 0xfffff000;
                            uint32_t old_pte = read_physical_address_4b(pt_old_phys + 4 * j);
                            physical_address_t mapping_phys = old_pte & 0xfffff000;
                            if (old_pte & 1)
                            {
                                LOG(TRACE, "Copying 0x%x-0x%x", 4096 * (j + i * 1024), 4095 + 4096 * (j + i * 1024));

                                physical_address_t page_address = task_virtual_address_space_create_page(&tasks[task_count - 1], i, j, PAGING_USER_LEVEL, true);

                                set_current_phys_mem_page(mapping_phys >> 12);
                                memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
                                set_current_phys_mem_page(page_address >> 12);
                                memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
                            }
                        }
                    }
                }

                // memcpy(tasks[task_count - 1].fpu_state, 
                //     tasks[current_task_index].fpu_state,
                //     sizeof(tasks[current_task_index].fpu_state));  

                *params = tasks[current_task_index].registers_data;
                switch_task(&params);
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
