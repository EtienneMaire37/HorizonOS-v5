#pragma once

#include "../multitasking/vas.h"
#include "../multitasking/task.c"

#include "kernel_panic.h"

#define is_a_valid_function(symbol_type) ((symbol_type) == 'T' || (symbol_type) == 'R' || (symbol_type) == 't' || (symbol_type) == 'r')  

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
            if (last_symbol_address <= eip && symbol_address > eip && is_a_valid_function(found_symbol_type))
            {
                putchar(file == kernel_symbols_file ? '[' : '(');
                CONTINUE_LOG(DEBUG, file == kernel_symbols_file ? "[" : "(");
                
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
                    CONTINUE_LOG(DEBUG, "%c", last_symbol_buffer[i]);
                    if (subfunction)
                    {
                        tty_set_color(light_tty_color & 0x07, light_tty_color & 0x70);
                        subfunction = false;
                    }
                }
                tty_set_color(FG_WHITE, BG_BLACK);
                putchar(file == kernel_symbols_file ? ']' : ')');
                CONTINUE_LOG(DEBUG, file == kernel_symbols_file ? "]" : ")");
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
                if (current_symbol_address != 0)    // && is_a_valid_function(current_symbol_type)
                {
                    last_symbol_address = symbol_address;
                    symbol_address = current_symbol_address;
                }
            }
            if (line_offset >= 11 && line_offset < 64 + 11) // && is_a_valid_function(current_symbol_type)
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

#define return_from_isr() do { current_phys_mem_page = old_phys_mem_page; return; } while (0)

void interrupt_handler(volatile struct interrupt_registers* registers)
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
            kernel_panic((struct interrupt_registers*)registers);
        }
        else
        {
            tasks[current_task_index].is_dead = true;
            switch_task(&registers);
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
            switch_task(&registers);
        return_from_isr();
    }
    if (registers->interrupt_number == 0xf0)  // System call
    {
        if (!multitasking_enabled || first_task_switch) return_from_isr();

        // #define LOG_SYSCALLS
        #ifdef LOG_SYSCALLS
        LOG(DEBUG, "Task \"%s\" (pid = %lu) sent system call %u", tasks[current_task_index].name, tasks[current_task_index].pid, registers->eax);
        #endif

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
        case SYSCALL_WRITE:     // * write | fildes = $ebx, buf = $ecx, nbyte = $edx | $eax = bytes_read, $ebx = errno
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
        case SYSCALL_GETPID:     // * getpid || $eax = pid_hi, $ebx = pid_lo
            registers->eax = tasks[current_task_index].pid >> 32;
            registers->ebx = (uint32_t)tasks[current_task_index].pid;
            break;
        case SYSCALL_FORK:     // * fork
            // if (task_count >= MAX_TASKS)
            if (true)
            {
                registers->eax = 0xffffffff;
                registers->ebx = 0xffffffff;   // -1
            }
            else
            {
                task_count++;

                const uint16_t new_task_index = task_count - 1;

                tasks[new_task_index].name = tasks[current_task_index].name;
                tasks[new_task_index].ring = tasks[current_task_index].ring;
                tasks[new_task_index].pid = current_pid++;
                tasks[new_task_index].system_task = tasks[current_task_index].system_task;
                tasks[new_task_index].reading_stdin = false;
                utf32_buffer_copy(&tasks[current_task_index].input_buffer, &tasks[new_task_index].input_buffer);

                tasks[new_task_index].cr3 = vas_create_empty();
                tasks[new_task_index].esp = tasks[current_task_index].esp;
                tasks[new_task_index].esp0 = tasks[current_task_index].esp0;

                copy_fpu_state(&tasks[current_task_index].fpu_state, &tasks[new_task_index].fpu_state);

                registers->eax = tasks[new_task_index].pid >> 32;
                registers->ebx = tasks[new_task_index].pid & 0xffffffff;

                
                for (uint16_t i = 0; i < 768; i++)
                {
                    uint32_t old_pde = read_physical_address_4b(tasks[current_task_index].cr3 + 4 * i);
                    uint32_t new_pde = read_physical_address_4b(tasks[new_task_index].cr3 + 4 * i);
                    if (!(old_pde & 1)) continue;
                    physical_address_t old_pt_address = old_pde & 0xfffff000;
                    physical_address_t new_pt_address = new_pde & 0xfffff000;
                    if (!(new_pde & 1))
                    {
                        new_pt_address = pfa_allocate_physical_page();
                        physical_init_page_table(new_pt_address);
                        write_physical_address_4b(tasks[new_task_index].cr3 + 4 * i, new_pt_address | (old_pde & 0xfff));
                    }
                    // LOG(TRACE, "%u : old_pt_address : 0x%lx", i, old_pt_address);
                    // LOG(TRACE, "%u : new_pt_address : 0x%lx", i, new_pt_address);
                    for (uint16_t j = (i == 0 ? 256 : 0); j < 1024; j++)
                    {
                        uint32_t old_pte = read_physical_address_4b(old_pt_address + 4 * j);
                        physical_address_t old_page_address = old_pte & 0xfffff000;
                        if (old_pte & 1)
                        {
                            uint32_t new_pte = read_physical_address_4b(new_pt_address + 4 * j);
                            physical_address_t new_page_address = new_pte & 0xfffff000;
                            if (!(new_pte & 1))
                            {
                                new_page_address = pfa_allocate_physical_page();
                                write_physical_address_4b(new_pt_address + 4 * j, new_page_address | (old_pte & 0xfff));
                            }
                            // LOG(TRACE, "%u.%u : old_page_address : 0x%lx", i, j, old_page_address);
                            // LOG(TRACE, "%u.%u : new_page_address : 0x%lx", i, j, new_page_address);
                            copy_page(old_page_address, new_page_address);
                        }
                    }
                }

                // disable_interrupts();

                abort();

            // //     asm volatile ("mov eax, esp" : "=a" (tasks[new_task_index].esp));

            // //     task_stack_push(&tasks[new_task_index], (uint32_t)&&ret);

            // //     task_stack_push(&tasks[new_task_index], registers->ebx);
            // //     task_stack_push(&tasks[new_task_index], registers->esi);
            // //     task_stack_push(&tasks[new_task_index], registers->edi);
            // //     task_stack_push(&tasks[new_task_index], registers->ebp);

            // //     full_context_switch(new_task_index);
            // //     goto old_task_ret;
            // // ret:
            // //     registers->eax = registers->ebx = 0;
            // // old_task_ret:
                // enable_interrupts();
            } 
            break;

        case SYSCALL_BRK_ALLOC: // * brk_alloc | address = $ebx | $eax = num_pages_allocated
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

                    // #define USE_IVLPG
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

        case SYSCALL_BRK_FREE: // * brk_free | address = $ebx | $eax = num_pages_freed
            {
                if (registers->ebx & 0xfff)
                {
                    registers->eax = 0;
                    break;
                }
                struct virtual_address_layout layout = *(struct virtual_address_layout*)&(registers->ebx);
                uint32_t pde = read_physical_address_4b(tasks[current_task_index].cr3 + 4 * layout.page_directory_entry);
                if (!(pde & 1))
                {
                    registers->eax = 0;
                    break;
                }                
                physical_address_t pt_address = (physical_address_t)pde & 0xfffff000;
                uint32_t pte = read_physical_address_4b(pt_address + 4 * layout.page_table_entry);
                if (!(pte & 1))
                    registers->eax = 0;
                else
                {
                    pfa_free_physical_page((physical_address_t)pte & 0xfffff000);
                    physical_remove_page(pt_address, layout.page_table_entry);

                    #ifdef USE_IVLPG
                    uint32_t* recursive_paging_pte = (uint32_t*)(((uint32_t)4 * 1024 * 1024 * 1023) | (4 * (layout.page_directory_entry * 1024 + layout.page_table_entry)));
                    *recursive_paging_pte = 0b1000;  // * Write-through caching | Not present

                    invlpg((uint32_t)recursive_paging_pte);
                    invlpg(4096 * (uint32_t)(layout.page_directory_entry * 1024 + layout.page_table_entry));
                    #else
                    load_pd_by_physaddr(tasks[current_task_index].cr3);
                    #endif

                    registers->eax = 1;
                }
            }
            break;

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
        #ifdef LOG_SYSCALLS
        LOG(TRACE, "Successfully handled syscall");
        #endif
    }

    return_from_isr();
}
