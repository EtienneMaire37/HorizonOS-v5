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

#define return_from_isr() { current_cr3 = params->cr3; return params->cr3; }

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
            // LOG(DEBUG, "Forking task \"%s\" (pid = %lu)", tasks[current_task_index].name, tasks[current_task_index].pid);
            if (true) // (task_count >= MAX_TASKS)
            {
                params->eax = 0xffffffff;
                params->ebx = 0xffffffff;   // -1
            }
            // else
            // {
            //     task_count++;

            //     tasks[task_count - 1].name = tasks[current_task_index].name;
            //     tasks[task_count - 1].ring = tasks[current_task_index].ring;

            //     if (tasks[current_task_index].stack_phys)
            //         tasks[task_count - 1].stack_phys = pfa_allocate_physical_page();
            //     if (tasks[current_task_index].kernel_stack_phys)
            //         tasks[task_count - 1].kernel_stack_phys = pfa_allocate_physical_page();
                    
            //     tasks[task_count - 1].registers = physical_address_to_virtual(tasks[task_count - 1].stack_phys + 4096) - sizeof(struct interrupt_registers);
            //     *(tasks[task_count - 1].registers) = *params;
            //     tasks[task_count - 1].registers->eax = tasks[task_count - 1].registers->ebx = 0;  // 0 to child
            //     tasks[task_count - 1].pid = current_pid++;

            //     params->eax = tasks[task_count - 1].pid >> 32;
            //     params->ebx = (uint32_t)tasks[task_count - 1].pid;

            //     // LOG(DEBUG, "Creating virtual address space");
            //     task_create_virtual_address_space(&tasks[task_count - 1]);
            //     if (tasks[current_task_index].kernel_stack_phys)
            //     {
            //         // LOG(DEBUG, "Copying kernel stack");
            //         // memcpy(tasks[task_count - 1].kernel_stack, tasks[current_task_index].kernel_stack, 4096);
            //         // load_pd(tasks[current_task_index].page_directory);
            //         // memcpy(page_tmp, (void*)TASK_KERNEL_STACK_BOTTOM_ADDRESS, 4096);
            //         // load_pd(tasks[task_count - 1].page_directory);
            //         // memcpy((void*)TASK_KERNEL_STACK_BOTTOM_ADDRESS, page_tmp, 4096);
            //         set_current_phys_mem_page(tasks[current_task_index].kernel_stack_phys >> 12);
            //         memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
            //         set_current_phys_mem_page(tasks[task_count - 1].kernel_stack_phys >> 12);
            //         memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
            //     }
            //     if (tasks[current_task_index].stack_phys)
            //     {
            //         // LOG(DEBUG, "Copying stack");
            //         // memcpy(tasks[task_count - 1].stack, tasks[current_task_index].stack, 4096);
            //         // load_pd(tasks[current_task_index].page_directory);
            //         // LOG(DEBUG, "Copying stack 1");
            //         // memcpy(page_tmp, (void*)TASK_STACK_BOTTOM_ADDRESS, 4096);
            //         // LOG(DEBUG, "Copying stack 2 0x%x", tasks[task_count - 1].page_directory);
            //         // load_pd(tasks[task_count - 1].page_directory);
            //         // LOG(DEBUG, "Copying stack 3");
            //         // memcpy((void*)TASK_STACK_BOTTOM_ADDRESS, page_tmp, 4096);
            //         set_current_phys_mem_page(tasks[current_task_index].stack_phys >> 12);
            //         memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
            //         set_current_phys_mem_page(tasks[task_count - 1].stack_phys >> 12);
            //         memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
            //     }
            //     // LOG(DEBUG, "Copying address space");
            //     for (uint16_t i = 0; i < 768; i++)
            //     {
            //         for (uint16_t j = ((i != 0) ? 0 : 256); j < ((i != 767) ? 1024 : 1021); j++)
            //         {
            //             if (tasks[current_task_index].page_directory[i].present)
            //             {
            //                 // struct virtual_address_layout layout;   // TODO: Do this properly with recursive paging
            //                 // layout.page_directory_entry = i;
            //                 // layout.page_table_entry = j;
            //                 // layout.page_offset = 0;
            //                 // struct page_table_entry* pt_n = (struct page_table_entry*)physical_address_to_virtual((physical_address_t)tasks[task_count - 1].page_directory[i].address << 12);
            //                 struct page_table_entry* pt_o = (struct page_table_entry*)physical_address_to_virtual((physical_address_t)tasks[current_task_index].page_directory[i].address << 12);
            //                 if (pt_o[i].present)
            //                 {
            //                     // LOG(DEBUG, "Copying 0x%x", *(uint32_t*)&layout);
            //                     physical_address_t page = task_virtual_address_space_create_page(&tasks[task_count - 1], i, j, PAGING_USER_LEVEL, true) >> 12;
            //                     // load_pd(tasks[current_task_index].page_directory);
            //                     // memcpy(page_tmp, (void*)(*(uint32_t*)&layout), 4096);
            //                     // load_pd(tasks[task_count - 1].page_directory);
            //                     // memcpy((void*)(*(uint32_t*)&layout), page_tmp, 4096);
            //                     set_current_phys_mem_page(pt_o->address);
            //                     memcpy(page_tmp, (void*)PHYS_MEM_PAGE_BOTTOM, 4096);
            //                     set_current_phys_mem_page(page);
            //                     memcpy((void*)PHYS_MEM_PAGE_BOTTOM, page_tmp, 4096);
            //                 }
            //             }
            //         }
            //     }
            //     tasks[task_count - 1].registers->cr3 = (uint32_t)virtual_address_to_physical((virtual_address_t)&tasks[task_count - 1].page_directory);
            // } 
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
