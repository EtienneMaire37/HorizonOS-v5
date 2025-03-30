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
    printf("Error:       %s\n\t", errorString[params->interrupt_number]);
    printf("Error code:  0x%x\n\n\t", params->error_code);

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

    current_cr3 = params->cr3; 
    current_phys_mem_page = 0xffffffff;

    if (params->interrupt_number < 32)            // Fault
    {
        LOG(ERROR, "Fault : Exception number : %u ; Error : %s ; Error code = 0x%x ; cr2 = 0x%x ; cr3 = 0x%x", params->interrupt_number, errorString[params->interrupt_number], params->error_code, params->cr2, params->cr3);

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
            if (true) // (task_count >= MAX_TASKS)
            {
                params->eax = 0xffffffff;
                params->ebx = 0xffffffff;   // -1
            }
            else
            {
                LOG(DEBUG, "Forking task \"%s\" (pid = %lu)", tasks[current_task_index].name, tasks[current_task_index].pid);

                // for (uint16_t i = 0; i < 1024; i++)
                // {
                //     LOG(DEBUG, "%u : 0x%x", i, ((uint32_t*)&page_directory)[i]);
                // }

                set_page(page_table_767, 1022, tasks[current_task_index].kernel_stack_phys, PAGING_SUPERVISOR_LEVEL, true);
                set_page(page_table_767, 1023, tasks[current_task_index].stack_phys, PAGING_SUPERVISOR_LEVEL, true);
                load_pd(page_directory);

                // LOG(DEBUG, ".");

                task_count++;

                tasks[task_count - 1].name = tasks[current_task_index].name;
                tasks[task_count - 1].ring = tasks[current_task_index].ring;
                tasks[task_count - 1].pid = current_pid++;

                if (tasks[current_task_index].stack_phys)
                    tasks[task_count - 1].stack_phys = pfa_allocate_physical_page();
                if (tasks[current_task_index].kernel_stack_phys)
                    tasks[task_count - 1].kernel_stack_phys = pfa_allocate_physical_page();

                if (tasks[current_task_index].kernel_stack_phys)
                {
                    set_current_phys_mem_page(tasks[current_task_index].kernel_stack_phys >> 12);
                    memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
                    set_current_phys_mem_page(tasks[task_count - 1].kernel_stack_phys >> 12);
                    memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
                }
                if (tasks[current_task_index].stack_phys)
                {
                    set_current_phys_mem_page(tasks[current_task_index].stack_phys >> 12);
                    memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
                    set_current_phys_mem_page(tasks[task_count - 1].stack_phys >> 12);
                    memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
                }

                task_create_virtual_address_space(&tasks[task_count - 1]);

                for (uint16_t i = 0; i < 768; i++)
                {
                    for (uint16_t j = (i == 0 ? 256 : 0); j < ((i != 767) ? 1024 : 1021); j++)
                    {
                        if (read_physical_address_4b(tasks[current_task_index].page_directory_phys + 4 * i) & 1)
                        {
                            physical_address_t pt_old_phys = read_physical_address_4b(tasks[current_task_index].page_directory_phys + 4 * i) & 0xfffff000;
                            if (read_physical_address_4b(pt_old_phys + 4 * j) & 1)
                            {
                                LOG(DEBUG, "Copying 0x%x-0x%x", 4096 * (j + i * 1024), 4095 + 4096 * (j + i * 1024));

                                physical_address_t page = task_virtual_address_space_create_page(&tasks[task_count - 1], i, j, PAGING_USER_LEVEL, true) >> 12;

                                set_current_phys_mem_page(read_physical_address_4b(pt_old_phys + 4 * j) >> 12);
                                memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
                                set_current_phys_mem_page(page);
                                memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
                            }
                        }
                    }
                }

                LOG(DEBUG, "Loading forked cr3 (0x%x)", tasks[task_count - 1].page_directory_phys);

                // load_pd_by_physaddr(tasks[task_count - 1].page_directory_phys);
                setting_cur_cr3 = true;
                current_cr3 = (uint32_t)tasks[task_count - 1].page_directory_phys;
                
                asm volatile("mov cr3, eax" : : "a" (current_cr3));     // !!!!!!!!!!! VERY IMPORTANT DON'T MESS UP THE STACK
                
                current_phys_mem_page = 0xffffffff;
                setting_cur_cr3 = false;

                LOG(DEBUG, "Copying registers");

                tasks[task_count - 1].registers = (struct interrupt_registers*)(TASK_KERNEL_STACK_TOP_ADDRESS - sizeof(struct interrupt_registers));

                *(tasks[task_count - 1].registers) = *params;
                tasks[task_count - 1].registers->eax = tasks[task_count - 1].registers->ebx = 0;  // 0 to child

                params->eax = tasks[task_count - 1].pid >> 32;
                params->ebx = (uint32_t)tasks[task_count - 1].pid;

                LOG(DEBUG, "Done");
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
