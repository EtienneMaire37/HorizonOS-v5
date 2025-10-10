#pragma once

// #define LOG_SYSCALLS
#define USE_IVLPG

void handle_syscall(interrupt_registers_t* registers)
{
    #ifdef LOG_SYSCALLS
    LOG(DEBUG, "Task \"%s\" (pid = %lu) sent system call %u", tasks[current_task_index].name, tasks[current_task_index].pid, registers->eax);
    #endif

    switch (registers->eax) // !! for some of these path resolution is handled in libc
    {
    case SYSCALL_EXIT:     // * exit | exit_code = $ebx |
        LOG(WARNING, "Task \"%s\" (pid = %lu) exited with return code %d", tasks[current_task_index].name, tasks[current_task_index].pid, registers->ebx);
        lock_task_queue();
        tasks[current_task_index].is_dead = true;
        unlock_task_queue();
        switch_task();
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
                    switch_task();
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
    case SYSCALL_WRITE:     // * write | fildes = $ebx, buf = $ecx, nbyte = $edx | $eax = bytes_written, $ebx = errno
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
        lock_task_queue();
        registers->eax = tasks[current_task_index].pid >> 32;
        registers->ebx = (uint32_t)tasks[current_task_index].pid;
        unlock_task_queue();
        break;
    case SYSCALL_FORK:     // * fork
        if (task_count >= MAX_TASKS)
        {
            registers->eax = 0xffffffff;
            registers->ebx = 0xffffffff;   // -1
        }
        else
        {
            lock_task_queue();
            tasks[current_task_index].forked_pid = current_pid++;
            pid_t forked_pid = tasks[current_task_index].forked_pid;
            unlock_task_queue();
            switch_task();
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
            uint32_t pte = read_physical_address_4b(pt_address + 4 * ((registers->ebx >> 12) & 0x3ff));
            if (!(pte & 1))
                registers->eax = 0;
            else
            {
                // LOG(DEBUG, "0x%x : 0x%lx", registers->ebx, (physical_address_t)pte & 0xfffff000);
                pfa_free_physical_page((physical_address_t)pte & 0xfffff000);
                physical_remove_page(pt_address, ((registers->ebx >> 12) & 0x3ff));

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

    case SYSCALL_EXECVE:    // * execve | path = $ebx, argv = $ecx, envp = $edx, cwd = $esi | $eax = errno
        {
            // LOG(DEBUG, "execve");
            startup_data_struct_t data = startup_data_init_from_argv((const char**)registers->ecx, (char**)registers->edx, (char*)registers->esi);
            // LOG(DEBUG, "execve.");
            lock_task_queue();
            if (!multitasking_add_task_from_vfs((char*)registers->ebx, (char*)registers->ebx, 3, false, &data))
            {
                unlock_task_queue();
                registers->eax = ENOENT;
                break;
            }
            else
            {
                pid_t old_pid = tasks[current_task_index].pid;
                tasks[current_task_index].is_dead = tasks[current_task_index].to_reap = true;
                tasks[task_count - 1].parent = tasks[current_task_index].parent;
                tasks[current_task_index].parent = -1;
                tasks[current_task_index].pid = tasks[task_count - 1].pid;
                tasks[task_count - 1].pid = old_pid;
                unlock_task_queue();
                switch_task();
                break;
            }
        }

    case SYSCALL_WAITPID: // * waitpid | pid_lo = $ebx, pid_hi = $ecx, options = $edx | $eax = errno, $ebx = *wstatus, $ecx = return_value[0:32], $edx = return_value[32:64]
        {
            uint64_t pid = ((uint64_t)registers->ecx << 32) | registers->ebx;
            lock_task_queue();
            tasks[current_task_index].wait_pid = *(pid_t*)&pid;
            unlock_task_queue();
            switch_task();
        }
        break;

    case SYSCALL_STAT:  // * stat | path = $ebx, stat_buf = $ecx | $eax = ret   
        {
            struct stat* st = (struct stat*)registers->ecx;
            const char* path = (const char*)registers->ebx;
            registers->eax = vfs_stat(path, st);
        }
        break;

    case SYSCALL_ACCESS:  // * access | path = $ebx, mode = $ecx | $eax = ret
        {
            const char* path = (const char*)registers->ebx;
            registers->eax = vfs_access(path, registers->ecx);
        }
        break;

    case SYSCALL_READDIR:   // * readdir | &dirent_entry = $ebx, dirp = $ecx | $eax = errno, $ebx = return_address
        {
            struct dirent* dirent_entry = (struct dirent*)registers->ebx;
            DIR* dirp = (DIR*)registers->ecx;
            registers->ebx = (uint32_t)vfs_readdir(dirent_entry, dirp);
            registers->eax = errno;
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
    // #define DEBUG_SYSCALLS
    #ifndef DEBUG_SYSCALLS
        tasks[current_task_index].is_dead = true;
        switch_task();
    #else
        abort();
    #endif
    }        
    #ifdef LOG_SYSCALLS
    LOG(TRACE, "Successfully handled syscall");
    #endif
}