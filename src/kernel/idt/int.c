#pragma once

#include "../multitasking/vas.h"
#include "../multitasking/task.c"

#include "kernel_panic.h"

#define return_from_isr() do { current_phys_mem_page = old_phys_mem_page; return; } while (0)

#define USE_IVLPG

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
            current_phys_mem_page = 0xffffffff;
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
                        current_phys_mem_page = 0xffffffff;
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
            if (task_count >= MAX_TASKS)
            {
                registers->eax = 0xffffffff;
                registers->ebx = 0xffffffff;   // -1
            }
            else
            {
                tasks[current_task_index].forked_pid = current_pid++;
                pid_t forked_pid = tasks[current_task_index].forked_pid;
                switch_task(&registers);
                current_phys_mem_page = 0xffffffff;
                if (tasks[current_task_index].pid == forked_pid)
                    registers->eax = registers->ebx = 0;
                else
                {
                    registers->eax = forked_pid >> 32;
                    registers->ebx = forked_pid & 0xffffffff;
                }
            } 
            break;

        case SYSCALL_BRK_ALLOC: // * brk_alloc | address = $ebx | $eax = num_pages_allocated
            {
                if (registers->ebx & 0xfff) // ! address not page aligned
                {
                    registers->eax = 0;
                    break;
                }
                
                uint32_t pde = read_physical_address_4b(tasks[current_task_index].cr3 + 4 * (registers->ebx >> 22));
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
                                            registers->ebx >> 22, 
                                            pt_address, 
                                            PAGING_USER_LEVEL, 
                                            true);
                }
                uint32_t pte = read_physical_address_4b(pt_address + 4 * ((registers->ebx >> 12) & 0x3ff));
                if (!(pte & 1))
                {
                    physical_address_t page = pfa_allocate_physical_page();
                    if (page == physical_null)
                    {
                        registers->eax = 0;
                        break;
                    }
                    physical_set_page(  pt_address, 
                                        ((registers->ebx >> 12) & 0x3ff), 
                                        page, 
                                        PAGING_USER_LEVEL, 
                                        true);
                    memset_page(page, 0);

                    #ifdef USE_IVLPG
                    uint32_t* recursive_paging_pte = (uint32_t*)(((uint32_t)4 * 1024 * 1024 * 1023) | (4 * ((registers->ebx >> 22) * 1024 + ((registers->ebx >> 12) & 0x3ff))));
                    *recursive_paging_pte = (page & 0xfffff000) | 0b1111;  // * Write-through caching | User level | Read write | Present

                    invlpg((uint32_t)recursive_paging_pte);
                    invlpg(4096 * (uint32_t)((registers->ebx >> 22) * 1024 + ((registers->ebx >> 12) & 0x3ff)));
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
                uint32_t pde = read_physical_address_4b(tasks[current_task_index].cr3 + 4 * (registers->ebx >> 22));
                if (!(pde & 1))
                {
                    registers->eax = 0;
                    break;
                }                
                physical_address_t pt_address = (physical_address_t)pde & 0xfffff000;
                uint32_t pte = read_physical_address_4b(pt_address + 4 * (registers->ebx >> 22));
                if (!(pte & 1))
                    registers->eax = 0;
                else
                {
                    pfa_free_physical_page((physical_address_t)pte & 0xfffff000);
                    physical_remove_page(pt_address, (registers->ebx >> 22));

                    #ifdef USE_IVLPG
                    uint32_t* recursive_paging_pte = (uint32_t*)(((uint32_t)4 * 1024 * 1024 * 1023) | (4 * ((registers->ebx >> 22) * 1024 + ((registers->ebx >> 12) & 0x3ff))));
                    *recursive_paging_pte = 0b1000;  // * Write-through caching | Not present

                    invlpg((uint32_t)recursive_paging_pte);
                    invlpg(4096 * (uint32_t)((registers->ebx >> 22) * 1024 + ((registers->ebx >> 12) & 0x3ff)));
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
            current_phys_mem_page = 0xffffffff;
        }        
        #ifdef LOG_SYSCALLS
        LOG(TRACE, "Successfully handled syscall");
        #endif
    }

    return_from_isr();
}
