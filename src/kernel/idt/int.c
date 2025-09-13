#pragma once

void kernel_panic(struct privilege_switch_interrupt_registers* registers)
{
    disable_interrupts();

    LOG(CRITICAL, "Kernel panic");

    tty_set_color(FG_WHITE, BG_BLACK);
    tty_clear_screen(' ');

    tty_cursor = 0;
    tty_update_cursor();
    // tty_hide_cursor();

    tty_set_color(FG_LIGHTRED, BG_BLACK);
    printf("Kernel panic\n\n");

    tty_set_color(FG_LIGHTGREEN, BG_BLACK);

    if (multitasking_enabled)
        printf("Task : \"%s\" (pid = %lu) | ring = %u\n\n", tasks[current_task_index].name, tasks[current_task_index].pid, tasks[current_task_index].ring);

    tty_set_color(FG_WHITE, BG_BLACK);

    printf("Exception number: %u\n", registers->interrupt_number);
    printf("Error:       %s\n", get_error_message(registers->interrupt_number, registers->error_code));
    printf("Error code:  0x%x\n\n", registers->error_code);

    if (registers->interrupt_number == 14)
    {
        printf("cr2:  0x%x (pde %u pte %u offset 0x%x)\n", registers->cr2, registers->cr2 >> 22, (registers->cr2 >> 12) & 0x3ff, registers->cr2 & 0xfff);
        printf("cr3:  0x%x\n\n", registers->cr3);

        uint32_t pde = read_physical_address_4b(current_cr3 + 4 * (registers->cr2 >> 22));
        
        LOG(DEBUG, "Page directory entry : 0x%x", pde);

        if (pde & 1)
        {
            LOG(DEBUG, "Page table entry : 0x%x", read_physical_address_4b((pde & 0xfffff000) + 4 * ((registers->cr2 >> 12) & 0x3ff)));
        }
    }

    printf("Stack trace : \n");
    LOG(DEBUG, "Stack trace : ");

    typedef struct call_frame
    {
        struct call_frame* ebp;
        uint32_t eip;
    } call_frame_t;

    call_frame_t* ebp = (call_frame_t*)registers->ebp;
    // asm volatile ("mov eax, ebp" : "=a"(ebp));

    // ~ Log the last function (the one the exception happened in)
    printf("eip : 0x%x ", registers->eip);
    LOG(DEBUG, "eip : 0x%x ", registers->eip);
    if ((((uint32_t)ebp > (uint32_t)&stack_bottom) && ((uint32_t)ebp <= (uint32_t)&stack_top)) || 
    (((uint32_t)ebp > TASK_STACK_BOTTOM_ADDRESS) && ((uint32_t)ebp <= TASK_STACK_TOP_ADDRESS)))
        print_kernel_symbol_name(registers->eip, (uint32_t)ebp);
    putchar('\n');

    while ((uint32_t)ebp != 0 && ((uint32_t)ebp != (uint32_t)&stack_top) && ((uint32_t)ebp != TASK_STACK_TOP_ADDRESS))
    {
        printf("eip : 0x%x | ebp : 0x%x ", ebp->eip, ebp);
        LOG(DEBUG, "eip : 0x%x | ebp : 0x%x ", ebp->eip, ebp);
        if ((((uint32_t)ebp > (uint32_t)&stack_bottom) && ((uint32_t)ebp <= (uint32_t)&stack_top)) || 
        (((uint32_t)ebp > TASK_STACK_BOTTOM_ADDRESS) && ((uint32_t)ebp <= TASK_STACK_TOP_ADDRESS)))
            print_kernel_symbol_name(ebp->eip - 1, (uint32_t)ebp); // ^ -1 cause it pushes the return address not the calling address
        putchar('\n');
        ebp = ebp->ebp;
    }

    halt();
}

void print_kernel_symbol_name(uint32_t eip, uint32_t ebp)
{
    initrd_file_t* file = ebp >= 0xc0000000 ? kernel_symbols_file : kernel_task_symbols_file;
    if (file == NULL) return;
    
    uint32_t symbol_address = 0, last_symbol_address = 0, current_symbol_address = 0;
    uint32_t file_offset = 0, line_offset = 0;
    char last_symbol_buffer[64] = {0};
    uint16_t last_symbol_buffer_length = 0;
    char current_symbol_type = ' ', found_symbol_type = ' ';
    while (file_offset < file->size)
    {
        char ch = file->data[file_offset];
        if (ch == '\n')
        {
            if (last_symbol_address <= eip && symbol_address > eip)
            {
                putchar(file == kernel_symbols_file ? '[' : '(');
                fputc(file == kernel_symbols_file ? '[' : '(', stderr);
                
                if (found_symbol_type == 'T' || found_symbol_type == 't')
                    tty_set_color(FG_LIGHTCYAN, BG_BLACK);
                else if (found_symbol_type == 'R' || found_symbol_type == 'r')
                    tty_set_color(FG_LIGHTMAGENTA, BG_BLACK);
                else
                    tty_set_color(FG_LIGHTGRAY, BG_BLACK);

                uint8_t light_tty_color = tty_color;
                bool subfunction = false;

                for (uint8_t i = 0; i < minint(64, last_symbol_buffer_length); i++)
                {
                    if (last_symbol_buffer[i] == '.')
                    {
                        tty_set_color(FG_LIGHTGRAY, BG_BLACK);
                        subfunction = true;
                    }
                    putchar(last_symbol_buffer[i]);
                    fputc(last_symbol_buffer[i], stderr);
                    if (subfunction)
                    {
                        tty_set_color(light_tty_color & 0x07, light_tty_color & 0x70);
                        subfunction = false;
                    }
                }
                tty_set_color(FG_WHITE, BG_BLACK);
                putchar(file == kernel_symbols_file ? ']' : ')');
                fputc(file == kernel_symbols_file ? ']' : ')', stderr);
                return;
            }
            else if (is_a_valid_function(current_symbol_type))
            {
                last_symbol_buffer_length = line_offset - 11;
            }
            line_offset = 0;
        }
        else
        {
            if (line_offset < 8)
            {
                uint32_t val = hex_char_to_int(ch);
                current_symbol_address &= ~((uint32_t)0xf << ((7 - line_offset) * 4));
                current_symbol_address |= val << ((7 - line_offset) * 4);
            }
            if (line_offset == 9)
            {
                current_symbol_type = ch;
                if (is_a_valid_function(current_symbol_type) && current_symbol_address != 0)
                {
                    last_symbol_address = symbol_address;
                    symbol_address = current_symbol_address;
                }
            }
            if (line_offset >= 11 && line_offset < 64 + 11 && is_a_valid_function(current_symbol_type))
            {
                if (!(last_symbol_address <= eip && symbol_address > eip))
                {
                    last_symbol_buffer[line_offset - 11] = ch;
                    found_symbol_type = current_symbol_type;
                }
            }
            line_offset++;
        }
        file_offset++;
    }
}

void __attribute__((cdecl)) interrupt_handler(struct privilege_switch_interrupt_registers* registers)
{
    current_cr3 = registers->cr3; 
    uint32_t old_phys_mem_page = current_phys_mem_page;
    current_phys_mem_page = 0xffffffff;

    if (registers->interrupt_number < 32)            // Fault
    {
        LOG(WARNING, "Fault : Exception number : %u ; Error : %s ; Error code = 0x%x ; cr2 = 0x%x ; cr3 = 0x%x", registers->interrupt_number, get_error_message(registers->interrupt_number, registers->error_code), registers->error_code, registers->cr2, registers->cr3);

        if (registers->interrupt_number == 6 && *((uint16_t*)registers->eip) == 0xa20f)  // Invalid Opcode + CPUID // ~ Assumes no instruction prefix // !! Also assumes that eip does not cross a non present page boundary
        {
            has_cpuid = false;
            return;
        }
        
        if (tasks[current_task_index].system_task || task_count == 1 || !multitasking_enabled || registers->interrupt_number == 8 || registers->interrupt_number == 18)
        // System task or last task or multitasking not enabled or Double Fault or Machine Check
            kernel_panic(registers);
        else
        {
            tasks[current_task_index].is_dead = true;
            switch_task(&registers);
        }

        return;
    }

    if (registers->interrupt_number < 32 + 16)  // IRQ
    {
        uint8_t irq_number = registers->interrupt_number - 32;

        if (irq_number == 7 && !(pic_get_isr() >> 7))
            return;
        if (irq_number == 15 && !(pic_get_isr() >> 15))
        {
            outb(PIC1_CMD, PIC_EOI);
	        io_wait();
            return;
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
            switch_task(&registers);
        return;
    }
    if (registers->interrupt_number == 0xf0)  // System call
    {
        // LOG(DEBUG, "Task \"%s\" (pid = %lu) sent system call %u", tasks[current_task_index].name, tasks[current_task_index].pid, registers->eax);
        if (!multitasking_enabled) return;

        uint16_t old_index;
        switch (registers->eax)
        {
        case SYSCALL_EXIT:     // * exit | exit_code = $ebx |
            LOG(WARNING, "Task \"%s\" (pid = %lu) exited with return code %d", tasks[current_task_index].name, tasks[current_task_index].pid, registers->ebx);
            tasks[current_task_index].is_dead = true;
            switch_task(&registers);
            break;
        case SYSCALL_TIME:     // * time || $eax = time
            registers->eax = time(NULL);
            break;
        case SYSCALL_READ:     // * read | fildes = $ebx, buf = $ecx, nbyte = $edx | $eax = bytes_read, $ebx = errno
            if (registers->ebx > 2) // ! Only default fds are supported for now
            {
                registers->eax = 0xffffffff;   // -1
                registers->ebx = EBADF;
            } 
            else
            {
                if (registers->ebx == STDIN_FILENO)
                {
                    registers->ebx = 0;
                    if (registers->edx == 0)
                    {
                        registers->eax = 0;
                        break;
                    }
                    if (no_buffered_characters(tasks[current_task_index].input_buffer))
                    {
                        tasks[current_task_index].reading_stdin = true;
                        switch_task(&registers);
                    }
                    registers->eax = minint(get_buffered_characters(tasks[current_task_index].input_buffer), registers->edx);
                    for (uint32_t i = 0; i < registers->eax; i++)
                    {
                        // *** Only ASCII for now ***
                        ((char*)registers->ecx)[i] = utf32_to_bios_oem(utf32_buffer_getchar(&tasks[current_task_index].input_buffer));
                    }
                }
                else
                {
                    registers->eax = 0xffffffff;   // -1
                    registers->ebx = EBADF;
                }
            }
            break;
        case SYSCALL_WRITE:     // write
            if (registers->ebx > 2)
            {
                registers->eax = 0xffffffff;   // -1
                registers->ebx = EBADF;
            }
            else
            {
                if (registers->ebx == STDOUT_FILENO || registers->ebx == STDERR_FILENO)
                {
                    for (uint32_t i = 0; i < registers->edx; i++)
                        tty_outc(((char*)registers->ecx)[i]);
                    tty_update_cursor();
                    registers->eax = registers->edx;
                }
                else
                {
                    registers->eax = 0xffffffff;
                    registers->ebx = EBADF;
                }
            }
            break;
        case SYSCALL_GETPID:     // getpid
            registers->eax = tasks[current_task_index].pid >> 32;
            registers->ebx = (uint32_t)tasks[current_task_index].pid;
            break;
        // case SYSCALL_FORK:     // fork
        //     LOG(DEBUG, "Forking task \"%s\" (pid = %lu)", tasks[current_task_index].name, tasks[current_task_index].pid);

        //     if (task_count >= MAX_TASKS)
        //     {
        //         registers->eax = 0xffffffff;
        //         registers->ebx = 0xffffffff;   // -1
        //     }
        //     else
        //     {
        //         task_count++;

        //         // tasks[current_task_index].registers_data = *registers;
        //         // tasks[current_task_index].registers_ptr = registers;

        //         tasks[task_count - 1].name = tasks[current_task_index].name;
        //         tasks[task_count - 1].ring = tasks[current_task_index].ring;
        //         tasks[task_count - 1].pid = current_pid++;
        //         tasks[task_count - 1].system_task = tasks[current_task_index].system_task;
        //         // tasks[task_count - 1].kernel_thread = tasks[current_task_index].kernel_thread;
        //         tasks[task_count - 1].reading_stdin = false;    // * Can't fork if the process is blocked
        //         utf32_buffer_init(&tasks[task_count - 1].input_buffer);

        //         if (tasks[current_task_index].stack_phys)
        //             tasks[task_count - 1].stack_phys = pfa_allocate_physical_page();
        //         if (tasks[current_task_index].kernel_stack_phys)
        //             tasks[task_count - 1].kernel_stack_phys = pfa_allocate_physical_page();

        //         // tasks[task_count - 1].registers_ptr = tasks[current_task_index].registers_ptr;
        //         // tasks[task_count - 1].registers_data = tasks[current_task_index].registers_data;

        //         task_create_virtual_address_space(&tasks[task_count - 1]);

        //         if (tasks[task_count - 1].kernel_stack_phys)
        //             copy_page(tasks[current_task_index].kernel_stack_phys, tasks[task_count - 1].kernel_stack_phys);
        //         if (tasks[task_count - 1].stack_phys)
        //             copy_page(tasks[current_task_index].stack_phys, tasks[task_count - 1].stack_phys);

        //         // tasks[task_count - 1].registers_data.eax = tasks[task_count - 1].registers_data.ebx = 0;
        //         // tasks[current_task_index].registers_data.eax = tasks[task_count - 1].pid >> 32;
        //         // tasks[current_task_index].registers_data.ebx = tasks[task_count - 1].pid;

        //         for (uint16_t i = 0; i < 768; i++)
        //         {
        //             uint32_t old_pde = read_physical_address_4b(tasks[current_task_index].cr3 + 4 * i);
        //             for (uint16_t j = (i == 0 ? 256 : 0); j < ((i == 767) ? 1021 : 1024); j++)
        //             {
        //                 if (old_pde & 1)
        //                 {
        //                     physical_address_t pt_old_phys = old_pde & 0xfffff000;
        //                     uint32_t old_pte = read_physical_address_4b(pt_old_phys + 4 * j);
        //                     physical_address_t mapping_phys = old_pte & 0xfffff000;
        //                     if (old_pte & 1)
        //                     {
        //                         // LOG(TRACE, "Copying 0x%x-0x%x", 4096 * (j + i * 1024), 4095 + 4096 * (j + i * 1024));

        //                         physical_address_t page_address = task_virtual_address_space_create_page(&tasks[task_count - 1], i, j, PAGING_USER_LEVEL, true);

        //                         copy_page(mapping_phys, page_address);
        //                     }
        //                 }
        //             }
        //         }

        //         // for (uint16_t i = 0; i < (tasks[task_count - 1].ring != 0 ? sizeof(struct privilege_switch_interrupt_registers) : sizeof(struct interrupt_registers)); i++)
        //         //     write_physical_address_1b((physical_address_t)((uint32_t)tasks[task_count - 1].registers_ptr) + tasks[task_count - 1].stack_phys - TASK_STACK_BOTTOM_ADDRESS + i,
        //         //         ((uint8_t*)&tasks[task_count - 1].registers_data)[i]);

        //         // if (tasks[current_task_index].ring != 0)
        //         // {
        //         //     registers->esp = tasks[current_task_index].registers_data.esp;
        //         //     registers->ss = tasks[current_task_index].registers_data.ss;
        //         // }

        //         // *(struct interrupt_registers*)registers = *(struct interrupt_registers*)&tasks[current_task_index].registers_data;

        //         switch_task(&registers);
        //     } 
        //     break;

        case SYSCALL_BRK_ALLOC:
            {
                if (registers->ebx & 0xfff) // ! address not page aligned
                {
                    registers->eax = 0;
                    break;
                }
                struct virtual_address_layout layout = *(struct virtual_address_layout*)&(registers->ebx);
                uint32_t pde = read_physical_address_4b(tasks[current_task_index].cr3 + 4 * layout.page_directory_entry);
                physical_address_t pt_address = (physical_address_t)pde & 0xfffff000;
                if (!(pde & 1))
                {
                    pt_address = pfa_allocate_physical_page();
                    if (pt_address == physical_null)
                    {
                        registers->eax = 0;
                        break;
                    }
                    physical_init_page_table(pt_address);
                    physical_add_page_table(tasks[current_task_index].cr3, 
                                            layout.page_directory_entry, 
                                            pt_address, 
                                            PAGING_USER_LEVEL, 
                                            true);
                }
                uint32_t pte = read_physical_address_4b(pt_address + 4 * layout.page_table_entry);
                if (!(pte & 1))
                {
                    physical_address_t page = pfa_allocate_physical_page();
                    if (page == physical_null)
                    {
                        registers->eax = 0;
                        break;
                    }
                    physical_set_page(  pt_address, 
                                        layout.page_table_entry, 
                                        page, 
                                        PAGING_USER_LEVEL, 
                                        true);
                    memset_page(page, 0);

                    #define USE_IVLPG
                    #ifdef USE_IVLPG
                    uint32_t* recursive_paging_pte = (uint32_t*)(((uint32_t)4 * 1024 * 1024 * 1023) | (4 * (layout.page_directory_entry * 1024 + layout.page_table_entry)));
                    *recursive_paging_pte = (page & 0xfffff000) | 0b1111;  // * Write-through caching | User level | Read write | Present

                    invlpg((uint32_t)recursive_paging_pte);
                    invlpg(4096 * (uint32_t)(layout.page_directory_entry * 1024 + layout.page_table_entry));
                    #else
                    load_pd_by_physaddr(tasks[current_task_index].cr3);
                    #endif
                    
                    registers->eax = 1;

                    // LOG(DEBUG, "Allocated page at address : 0x%x", registers->ebx);
                }
                else
                    registers->eax = 0;
            }
            break;

        // case SYSCALL_BRK_FREE:
        //     {
        //         if (registers->ebx & 0xfff)
        //         {
        //             registers->eax = 0;
        //             break;
        //         }
        //         struct virtual_address_layout layout = *(struct virtual_address_layout*)&(registers->ebx);
        //         uint32_t pde = read_physical_address_4b(tasks[current_task_index].cr3 + 4 * layout.page_directory_entry);
        //         if (!(pde & 1))
        //         {
        //             registers->eax = 0;
        //             break;
        //         }                
        //         physical_address_t pt_address = (physical_address_t)pde & 0xfffff000;
        //         uint32_t pte = read_physical_address_4b(pt_address + 4 * layout.page_table_entry);
        //         if (!(pte & 1))
        //             registers->eax = 0;
        //         else
        //         {
        //             pfa_free_physical_page((physical_address_t)pte & 0xfffff000);
        //             physical_remove_page(pt_address, layout.page_table_entry);
        //             flush_tlb = true;
        //             registers->eax = 1;
        //         }
        //     }
        //     break;

        case SYSCALL_FLUSH_INPUT_BUFFER:
            utf32_buffer_clear(&(tasks[current_task_index].input_buffer));
            break;

        case SYSCALL_SET_KB_LAYOUT:
            if (tasks[current_task_index].ring == 0 && registers->ebx >= 1 && registers->ebx <= NUM_KB_LAYOUTS)
            {
                current_keyboard_layout = keyboard_layouts[registers->ebx - 1];
                registers->eax = 1;
            }
            else
                registers->eax = 0;
            break;

        default:
            LOG(ERROR, "Undefined system call (0x%x)", registers->eax);
            tasks[current_task_index].is_dead = true;
            switch_task(&registers);
        }            
    }

    return;
}
