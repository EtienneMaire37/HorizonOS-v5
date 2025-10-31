#pragma once

// #define LOG_SYSCALLS
#define USE_IVLPG

void handle_syscall(interrupt_registers_t* registers)
{
    #ifdef LOG_SYSCALLS
    LOG(DEBUG, "Task \"%s\" (pid = %d) sent system call %u", tasks[current_task_index].name, tasks[current_task_index].pid, registers->eax);
    #endif

    switch (registers->eax) // !! for some of these path resolution is handled in libc
    {
    case SYSCALL_EXIT:     // * exit | exit_code = $ebx |
        LOG(WARNING, "Task \"%s\" (pid = %d) exited with return code %d", tasks[current_task_index].name, tasks[current_task_index].pid, registers->ebx);
        lock_task_queue();
        tasks[current_task_index].is_dead = true;
        tasks[current_task_index].return_value = registers->ebx & 0xff;
        unlock_task_queue();
        switch_task();
        break;
    case SYSCALL_TIME:      // * time || $eax = time
        registers->eax = ktime(NULL);
        break;

    case SYSCALL_OPEN:      // * open | path = $ebx, oflag = $ecx, mode = $edx | $eax = errno, $ebx = fd
    {
        const char* path = (const char*)registers->ebx;

        int fd = vfs_allocate_global_file();
        file_table[fd].type = get_drive_type(path);
        file_table[fd].flags = (*(int*)&registers->ecx) & (O_CLOEXEC | O_RDONLY | O_RDWR | O_WRONLY);   // * | O_APPEND | O_CREAT
        file_table[fd].position = 0;

        if (file_table[fd].flags != *(int*)&registers->ecx)
        {
            vfs_remove_global_file(fd);
            registers->ebx = 0xffffffff;
            registers->eax = EINVAL;
            break;
        }

        struct stat st;
        int stat_ret = vfs_stat(path, &st);
        if (stat_ret != 0)
        {
            vfs_remove_global_file(fd);
            registers->ebx = 0xffffffff;
            registers->eax = stat_ret;
            break;
        }

        if ((!(st.st_mode & S_IRUSR)) && ((file_table[fd].flags & O_RDWR) | (file_table[fd].flags & O_RDONLY))) // * Assume we're the owner of every file
        {
            vfs_remove_global_file(fd);
            registers->ebx = 0xffffffff;
            registers->eax = EACCES;
            break;
        }

        if ((!(st.st_mode & S_IWUSR)) && ((file_table[fd].flags & O_RDWR) | (file_table[fd].flags & O_WRONLY))) // * Assume we're the owner of every file
        {
            vfs_remove_global_file(fd);
            registers->ebx = 0xffffffff;
            registers->eax = EACCES;
            break;
        }

        file_table[fd].flags &= ~(O_APPEND | O_CREAT);
        switch (file_table[fd].type)
        {
        case DT_INITRD:
            file_table[fd].data.initrd_data.file = initrd_find_file_entry((char*)path + 
                            strlen("/initrd") + (strlen(path) > strlen("/initrd") ? 1 : 0));
            break;
        default:
            file_table[fd].type = DT_INVALID;
        }
        int ret = vfs_allocate_thread_file(current_task_index);
        // LOG(DEBUG, "global fd : %d", fd);
        // LOG(DEBUG, "fd : %d", ret);
        if (ret == -1)
        {
            vfs_remove_global_file(fd);

            registers->ebx = 0xffffffff;
            registers->eax = ENOMEM;
        }
        else
        {
            tasks[current_task_index].file_table[ret] = fd;
            registers->ebx = *(uint32_t*)&ret;
            registers->eax = 0;
        }
        break;
    }

    case SYSCALL_CLOSE:     // * close | fildes = $ebx | $eax = errno, $ebx = ret
        int fd = *(int*)&registers->ebx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->ebx = 0xffffffff;
            registers->eax = EBADF;
            break;
        }
        if (tasks[current_task_index].file_table[fd] == invalid_fd)
        {
            registers->ebx = 0xffffffff;
            registers->eax = EBADF;
            break;
        }
        vfs_remove_global_file(tasks[current_task_index].file_table[fd]);
        tasks[current_task_index].file_table[fd] = invalid_fd;
        break;

    case SYSCALL_READ:      // * read | fildes = $ebx, buf = $ecx, nbyte = $edx | $eax = bytes_read, $ebx = errno
    {
        int fd = *(int*)&registers->ebx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->eax = 0xffffffff;
            registers->ebx = EBADF;
            break;
        }
        if (tasks[current_task_index].file_table[fd] == invalid_fd)
        {
            registers->eax = 0xffffffff;
            registers->ebx = EBADF;
            break;
        }
        if (tasks[current_task_index].file_table[fd] > 2)
        {
            file_entry_t* entry = &file_table[tasks[current_task_index].file_table[fd]];
            ssize_t bytes_read;
            registers->ebx = vfs_read(entry, (void*)registers->ecx, registers->edx, &bytes_read);
            registers->eax = *(uint32_t*)&bytes_read;
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
    }
    case SYSCALL_WRITE:     // * write | fildes = $ebx, buf = $ecx, nbyte = $edx | $eax = bytes_written, $ebx = errno
    {
        int fd = *(int*)&registers->ebx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->eax = 0xffffffff;
            registers->ebx = EBADF;
            break;
        }
        if (tasks[current_task_index].file_table[fd] == invalid_fd)
        {
            registers->eax = 0xffffffff;
            registers->ebx = EBADF;
            break;
        }
        if (tasks[current_task_index].file_table[fd] > 2)   // ! Only default fds are supported for now
        {
            registers->eax = 0;
            registers->ebx = 0;
        }
        else
        {
            if (registers->ebx == STDOUT_FILENO || registers->ebx == STDERR_FILENO)
            {
                for (uint32_t i = 0; i < registers->edx; i++)
                    tty_outc(((char*)registers->ecx)[i]);
                tty_update_cursor();
                registers->eax = registers->edx;    // bytes_written
            }
            else    // ! cant write to STDIN_FILENO
            {
                registers->eax = 0xffffffff;
                registers->ebx = EBADF;
            }
        }
        break;
    }
    case SYSCALL_GETPID:     // * getpid || $eax = pid
        lock_task_queue();
        registers->eax = tasks[current_task_index].pid;
        unlock_task_queue();
        break;
    case SYSCALL_FORK:     // * fork
        if (task_count >= MAX_TASKS)
        {
            registers->eax = 0xffffffff;   // -1
        }
        else
        {
            lock_task_queue();
            tasks[current_task_index].forked_pid = current_pid++;
            pid_t forked_pid = tasks[current_task_index].forked_pid;
            unlock_task_queue();
            switch_task();
            if (tasks[current_task_index].pid == forked_pid)
                registers->eax = 0;
            else
            {
                registers->eax = forked_pid;
            }
        } 
        break;

    case SYSCALL_EXECVE:    // * execve | path = $ebx, argv = $ecx, envp = $edx, cwd = $esi | $eax = errno
    {
        if (vfs_access((char*)registers->ebx, X_OK) != 0)
        {
            registers->eax = EACCES;
            break;
        }
        startup_data_struct_t data = startup_data_init_from_argv((const char**)registers->ecx, (char**)registers->edx, (char*)registers->esi);
        lock_task_queue();
        if (!multitasking_add_task_from_vfs((char*)registers->ebx, (char*)registers->ebx, 3, false, &data))
        {
            unlock_task_queue();
            registers->eax = ENOENT;
            break;
        }
        else
        {
            uint16_t new_task_index = task_count - 1;
            pid_t old_pid = tasks[current_task_index].pid;
            tasks[current_task_index].is_dead = tasks[current_task_index].to_reap = true;
            tasks[new_task_index].parent = tasks[current_task_index].parent;
            tasks[current_task_index].parent = -1;
            tasks[current_task_index].pid = tasks[new_task_index].pid;
            tasks[new_task_index].pid = old_pid;

            task_copy_file_table(current_task_index, new_task_index, true);

            unlock_task_queue();
            switch_task();
            break;
        }
    }

    case SYSCALL_WAITPID: // * waitpid | pid = $ebx, options = $edx | $eax = errno, $ebx = *wstatus, $ecx = return_value
    {
        pid_t pid = registers->ecx;
        lock_task_queue();
        tasks[current_task_index].wait_pid = registers->ebx;
        unlock_task_queue();
        switch_task();
        registers->ecx = pid;
        registers->ebx = tasks[current_task_index].wstatus;
        break;
    }

    case SYSCALL_STAT:  // * stat | path = $ebx, stat_buf = $ecx | $eax = ret   
    {
        struct stat* st = (struct stat*)registers->ecx;
        const char* path = (const char*)registers->ebx;
        registers->eax = vfs_stat(path, st);
        break;
    }

    case SYSCALL_ACCESS:  // * access | path = $ebx, mode = $ecx | $eax = ret
    {
        const char* path = (const char*)registers->ebx;
        registers->eax = vfs_access(path, registers->ecx);
        break;
    }

    case SYSCALL_READDIR:   // * readdir | &dirent_entry = $ebx, dirp = $ecx | $eax = errno, $ebx = return_address
    {
        struct dirent* dirent_entry = (struct dirent*)registers->ebx;
        DIR* dirp = (DIR*)registers->ecx;
        registers->ebx = (uint32_t)vfs_readdir(dirent_entry, dirp);
        registers->eax = errno;
        break;
    }

    case SYSCALL_ISATTY:    // * isatty | fd = $ebx | $eax = errno, $ebx = ret
    {
        int fd = *(int*)&registers->ebx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->eax = EBADF;
            registers->ebx = 0;
            break;
        }
        if (tasks[current_task_index].file_table[fd] == invalid_fd)
        {
            registers->eax = EBADF;
            registers->ebx = 0;
            break;
        }
        registers->ebx = tasks[current_task_index].file_table[fd] < 3;
        if (!registers->ebx)
            registers->eax = ENOTTY;
        break;
    }

    case SYSCALL_TCGETATTR:     // * tcgetattr | fildes = $ebx, termios_p = $ecx | $eax = errno
    {
        struct termios* termios_p = (struct termios*)registers->ecx;
        if (!termios_p)
        {
            registers->eax = EINVAL;
            break;
        }
        int fd = *(int*)&registers->ebx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->eax = EBADF;
            break;
        }
        if (tasks[current_task_index].file_table[fd] == invalid_fd)
        {
            registers->eax = EBADF;
            break;
        }
        if (file_table[tasks[current_task_index].file_table[fd]].type != DT_TERMINAL)  // ! not a tty
        {
            registers->eax = ENOTTY;
            break;
        }
        *termios_p = file_table[tasks[current_task_index].file_table[fd]].data.terminal_data.ts;
        registers->eax = 0;
        break;
    }

    case SYSCALL_TCSETATTR:     // * tcsetattr | fildes = $ebx, termios_p = $ecx, optional_actions = $edx | $eax = errno
    {
        struct termios* termios_p = (struct termios*)registers->ecx;
        if (!termios_p)
        {
            registers->eax = EINVAL;
            break;
        }
        int fd = *(int*)&registers->ebx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->eax = EBADF;
            break;
        }
        if (tasks[current_task_index].file_table[fd] == invalid_fd)
        {
            registers->eax = EBADF;
            break;
        }
        if (file_table[tasks[current_task_index].file_table[fd]].type != DT_TERMINAL)  // ! not a tty
        {
            registers->eax = ENOTTY;
            break;
        }
        file_table[tasks[current_task_index].file_table[fd]].data.terminal_data.ts = *termios_p;
        registers->eax = 0;
        break;
    }

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

    case SYSCALL_BRK_ALLOC: // * brk_alloc | address = $ebx | $eax = num_pages_allocated
        {
            // LOG(DEBUG, "alloc 0x%x", registers->ebx);

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
            // LOG(DEBUG, "free 0x%x", registers->ebx);

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

    default:
        LOG(ERROR, "Undefined system call (0x%x)", registers->eax);
    // #define DEBUG_SYSCALLS
    #ifndef DEBUG_SYSCALLS
        tasks[current_task_index].is_dead = true;
        tasks[current_task_index].return_value = 0x80000000;
        switch_task();
    #else
        abort();
    #endif
    }        
    #ifdef LOG_SYSCALLS
    LOG(TRACE, "Successfully handled syscall");
    #endif
}