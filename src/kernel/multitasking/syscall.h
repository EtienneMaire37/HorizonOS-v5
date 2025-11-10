#pragma once

#include "../int/int.h"
#include "../../libc/src/syscall_defines.h"

void handle_syscall(interrupt_registers_t* registers)
{
    switch (registers->rax) // !! some of the path resolution is handled in libc
    {
    case SYSCALL_EXIT:     // * exit | exit_code = $rbx |
        LOG(WARNING, "Task \"%s\" (pid = %d) exited with return code %d", __CURRENT_TASK.name, __CURRENT_TASK.pid, (int)registers->rbx);
        lock_task_queue();
        __CURRENT_TASK.is_dead = true;
        __CURRENT_TASK.return_value = registers->rbx & 0xff;
        unlock_task_queue();
        switch_task();
        break;

    case SYSCALL_OPEN:      // * open | path = $rbx, oflag = $rcx, mode = $rdx | $rax = errno, $rbx = fd
    {
        const char* path = (const char*)registers->rbx;

        int fd = vfs_allocate_global_file();
        file_table[fd].type = get_drive_type(path);
        file_table[fd].flags = ((int)registers->rcx) & (O_CLOEXEC | O_RDONLY | O_RDWR | O_WRONLY);   // * | O_APPEND | O_CREAT
        file_table[fd].position = 0;

        if (file_table[fd].flags != (int)registers->rcx)
        {
            vfs_remove_global_file(fd);
            registers->rbx = (uint64_t)(-1);
            registers->rax = EINVAL;
            break;
        }

        struct stat st;
        int stat_ret = vfs_stat(path, &st);
        if (stat_ret != 0)
        {
            vfs_remove_global_file(fd);
            registers->rbx = (uint64_t)(-1);
            registers->rax = stat_ret;
            break;
        }

        if ((!(st.st_mode & S_IRUSR)) && ((file_table[fd].flags & O_RDWR) | (file_table[fd].flags & O_RDONLY))) // * Assume we're the owner of every file
        {
            vfs_remove_global_file(fd);
            registers->rbx = (uint64_t)(-1);
            registers->rax = EACCES;
            break;
        }

        if ((!(st.st_mode & S_IWUSR)) && ((file_table[fd].flags & O_RDWR) | (file_table[fd].flags & O_WRONLY))) // * Assume we're the owner of every file
        {
            vfs_remove_global_file(fd);
            registers->rbx = (uint64_t)(-1);
            registers->rax = EACCES;
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

            registers->rbx = (uint64_t)(-1);
            registers->rax = ENOMEM;
        }
        else
        {
            __CURRENT_TASK.file_table[ret] = fd;
            registers->rbx = *(uint32_t*)&ret;
            registers->rax = 0;
        }
        break;
    }

    case SYSCALL_CLOSE:     // * close | fildes = $rbx | $rax = errno, $rbx = ret
        int fd = (int)registers->rbx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->rbx = (uint64_t)(-1);
            registers->rax = EBADF;
            break;
        }
        if (__CURRENT_TASK.file_table[fd] == invalid_fd)
        {
            registers->rbx = (uint64_t)(-1);
            registers->rax = EBADF;
            break;
        }
        vfs_remove_global_file(__CURRENT_TASK.file_table[fd]);
        __CURRENT_TASK.file_table[fd] = invalid_fd;
        break;

    case SYSCALL_WRITE:     // * write | fildes = $rbx, buf = $rcx, nbyte = $rdx | $rax = bytes_written, $rbx = errno
    {
        int fd = registers->rbx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->rax = (uint64_t)(-1);
            registers->rbx = EBADF;
            break;
        }
        if (__CURRENT_TASK.file_table[fd] == invalid_fd)
        {
            registers->rax = (uint64_t)(-1);
            registers->rbx = EBADF;
            break;
        }
        if (__CURRENT_TASK.file_table[fd] > 2)   // ! Only default fds are supported for now
        {
            registers->rax = 0;
            registers->rbx = 0;
        }
        else
        {
            if (registers->rbx == STDOUT_FILENO || registers->rbx == STDERR_FILENO)
            {
                for (uint32_t i = 0; i < registers->rdx; i++)
                    tty_outc(((char*)registers->rcx)[i]);
                tty_update_cursor();
                registers->rax = registers->rdx;    // bytes_written
            }
            else    // ! cant write to STDIN_FILENO
            {
                registers->rax = (uint64_t)(-1);
                registers->rbx = EBADF;
            }
        }
        break;
    }

    case SYSCALL_READ:      // * read | fildes = $rbx, buf = $rcx, nbyte = $rdx | $rax = bytes_read, $rbx = errno
    {
        int fd = (int)registers->rbx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->rax = (uint64_t)-1;
            registers->rbx = EBADF;
            break;
        }
        if (__CURRENT_TASK.file_table[fd] == invalid_fd)
        {
            registers->rax = (uint64_t)-1;
            registers->rbx = EBADF;
            break;
        }
        if (__CURRENT_TASK.file_table[fd] > 2)
        {
            file_entry_t* entry = &file_table[__CURRENT_TASK.file_table[fd]];
            ssize_t bytes_read;
            registers->rbx = vfs_read(entry, (void*)registers->rcx, registers->rdx, &bytes_read);
            registers->rax = (uint32_t)bytes_read;
        } 
        else
        {
            if (registers->rbx == STDIN_FILENO)
            {
                registers->rbx = 0;
                if (registers->rdx == 0)
                {
                    registers->rax = 0;
                    break;
                }
                if (no_buffered_characters(__CURRENT_TASK.input_buffer))
                {
                    __CURRENT_TASK.reading_stdin = true;
                    switch_task();
                }
                registers->rax = minint(get_buffered_characters(__CURRENT_TASK.input_buffer), registers->r12);
                for (uint32_t i = 0; i < registers->rax; i++)
                {
                    // *** Only ASCII for now ***
                    ((char*)registers->rcx)[i] = utf32_to_bios_oem(utf32_buffer_getchar(&__CURRENT_TASK.input_buffer));
                }
            }
            else
            {
                registers->rax = (uint64_t)-1;
                registers->rbx = EBADF;
            }
        }
        break;
    }

    case SYSCALL_BRK:   // * brk | addr = $rbx, break_address = $rcx | $rax = errno, $rbx = break_address
    {
        uint64_t addr = registers->rdx;
        uint64_t break_address = registers->rcx;

        if (addr < break_address)
        {
            free_range((uint64_t*)__CURRENT_TASK.cr3, ((break_address + 0xfff) & ~0xfffULL), (break_address - addr + 0xfff) / 0x1000);
        }
        else
        {
            allocate_range((uint64_t*)__CURRENT_TASK.cr3, 
                    break_address & ~0xfffULL, (addr - break_address + 0xfff) / 0x1000, 
                    __CURRENT_TASK.ring == 3 ? PG_USER : PG_SUPERVISOR, 
                    PG_READ_WRITE, CACHE_WB);
        }

        registers->rbx = break_address;
        registers->rax = break_address == addr ? 0 : ENOMEM;
        break;
    }

    case SYSCALL_ISATTY:    // * isatty | fd = $rbx | $rax = errno, $rbx = ret
    {
        int fd = (int)registers->rbx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->rax = EBADF;
            registers->rbx = 0;
            break;
        }
        if (__CURRENT_TASK.file_table[fd] == invalid_fd)
        {
            registers->rax = EBADF;
            registers->rbx = 0;
            break;
        }
        registers->rbx = __CURRENT_TASK.file_table[fd] < 3;
        if (!registers->rbx)
            registers->rax = ENOTTY;
        break;
    }

    default:
        LOG(ERROR, "Undefined system call (0x%llx)", registers->rax);
        
        __CURRENT_TASK.is_dead = true;
        __CURRENT_TASK.return_value = 0x80000000;
        switch_task();
    }
}

// void handle_syscall(interrupt_registers_t* registers)
// {
//     #ifdef LOG_SYSCALLS
//     LOG(DEBUG, "Task \"%s\" (pid = %d) sent system call %llu", __CURRENT_TASK.name, __CURRENT_TASK.pid, registers->eax);
//     #endif

//     switch (registers->eax) // !! some of these path resolution is handled in libc
//     {
//     case SYSCALL_EXIT:     // * exit | exit_code = $ebx |
//         LOG(WARNING, "Task \"%s\" (pid = %d) exited with return code %d", __CURRENT_TASK.name, __CURRENT_TASK.pid, registers->ebx);
//         lock_task_queue();
//         __CURRENT_TASK.is_dead = true;
//         __CURRENT_TASK.return_value = registers->ebx & 0xff;
//         unlock_task_queue();
//         switch_task();
//         break;
//     case SYSCALL_TIME:      // * time || $eax = time
//         registers->eax = ktime(NULL);
//         break;

//     case SYSCALL_OPEN:      // * open | path = $ebx, oflag = $ecx, mode = $edx | $eax = errno, $ebx = fd
//     {
//         const char* path = (const char*)registers->ebx;

//         int fd = vfs_allocate_global_file();
//         file_table[fd].type = get_drive_type(path);
//         file_table[fd].flags = (*(int*)&registers->ecx) & (O_CLOEXEC | O_RDONLY | O_RDWR | O_WRONLY);   // * | O_APPEND | O_CREAT
//         file_table[fd].position = 0;

//         if (file_table[fd].flags != *(int*)&registers->ecx)
//         {
//             vfs_remove_global_file(fd);
//             registers->ebx = 0xffffffff;
//             registers->eax = EINVAL;
//             break;
//         }

//         struct stat st;
//         int stat_ret = vfs_stat(path, &st);
//         if (stat_ret != 0)
//         {
//             vfs_remove_global_file(fd);
//             registers->ebx = 0xffffffff;
//             registers->eax = stat_ret;
//             break;
//         }

//         if ((!(st.st_mode & S_IRUSR)) && ((file_table[fd].flags & O_RDWR) | (file_table[fd].flags & O_RDONLY))) // * Assume we're the owner of every file
//         {
//             vfs_remove_global_file(fd);
//             registers->ebx = 0xffffffff;
//             registers->eax = EACCES;
//             break;
//         }

//         if ((!(st.st_mode & S_IWUSR)) && ((file_table[fd].flags & O_RDWR) | (file_table[fd].flags & O_WRONLY))) // * Assume we're the owner of every file
//         {
//             vfs_remove_global_file(fd);
//             registers->ebx = 0xffffffff;
//             registers->eax = EACCES;
//             break;
//         }

//         file_table[fd].flags &= ~(O_APPEND | O_CREAT);
//         switch (file_table[fd].type)
//         {
//         case DT_INITRD:
//             file_table[fd].data.initrd_data.file = initrd_find_file_entry((char*)path + 
//                             strlen("/initrd") + (strlen(path) > strlen("/initrd") ? 1 : 0));
//             break;
//         default:
//             file_table[fd].type = DT_INVALID;
//         }
//         int ret = vfs_allocate_thread_file(current_task_index);
//         // LOG(DEBUG, "global fd : %d", fd);
//         // LOG(DEBUG, "fd : %d", ret);
//         if (ret == -1)
//         {
//             vfs_remove_global_file(fd);

//             registers->ebx = 0xffffffff;
//             registers->eax = ENOMEM;
//         }
//         else
//         {
//             __CURRENT_TASK.file_table[ret] = fd;
//             registers->ebx = *(uint32_t*)&ret;
//             registers->eax = 0;
//         }
//         break;
//     }

//     case SYSCALL_CLOSE:     // * close | fildes = $ebx | $eax = errno, $ebx = ret
//         int fd = *(int*)&registers->ebx;
//         if (fd < 0 || fd >= OPEN_MAX)
//         {
//             registers->ebx = 0xffffffff;
//             registers->eax = EBADF;
//             break;
//         }
//         if (__CURRENT_TASK.file_table[fd] == invalid_fd)
//         {
//             registers->ebx = 0xffffffff;
//             registers->eax = EBADF;
//             break;
//         }
//         vfs_remove_global_file(__CURRENT_TASK.file_table[fd]);
//         __CURRENT_TASK.file_table[fd] = invalid_fd;
//         break;
//     case SYSCALL_WRITE:     // * write | fildes = $ebx, buf = $ecx, nbyte = $edx | $eax = bytes_written, $ebx = errno
//     {
//         int fd = *(int*)&registers->ebx;
//         if (fd < 0 || fd >= OPEN_MAX)
//         {
//             registers->eax = 0xffffffff;
//             registers->ebx = EBADF;
//             break;
//         }
//         if (__CURRENT_TASK.file_table[fd] == invalid_fd)
//         {
//             registers->eax = 0xffffffff;
//             registers->ebx = EBADF;
//             break;
//         }
//         if (__CURRENT_TASK.file_table[fd] > 2)   // ! Only default fds are supported for now
//         {
//             registers->eax = 0;
//             registers->ebx = 0;
//         }
//         else
//         {
//             if (registers->ebx == STDOUT_FILENO || registers->ebx == STDERR_FILENO)
//             {
//                 for (uint32_t i = 0; i < registers->edx; i++)
//                     tty_outc(((char*)registers->ecx)[i]);
//                 tty_update_cursor();
//                 registers->eax = registers->edx;    // bytes_written
//             }
//             else    // ! cant write to STDIN_FILENO
//             {
//                 registers->eax = 0xffffffff;
//                 registers->ebx = EBADF;
//             }
//         }
//         break;
//     }
//     case SYSCALL_GETPID:     // * getpid || $eax = pid
//         lock_task_queue();
//         registers->eax = __CURRENT_TASK.pid;
//         unlock_task_queue();
//         break;
//     case SYSCALL_FORK:     // * fork
//         if (task_count >= MAX_TASKS)
//         {
//             registers->eax = 0xffffffff;   // -1
//         }
//         else
//         {
//             lock_task_queue();
//             __CURRENT_TASK.forked_pid = current_pid++;
//             pid_t forked_pid = __CURRENT_TASK.forked_pid;
//             unlock_task_queue();
//             switch_task();
//             if (__CURRENT_TASK.pid == forked_pid)
//                 registers->eax = 0;
//             else
//             {
//                 registers->eax = forked_pid;
//             }
//         } 
//         break;

//     case SYSCALL_EXECVE:    // * execve | path = $ebx, argv = $ecx, envp = $edx, cwd = $esi | $eax = errno
//     {
//         if (vfs_access((char*)registers->ebx, X_OK) != 0)
//         {
//             registers->eax = EACCES;
//             break;
//         }
//         startup_data_struct_t data = startup_data_init_from_argv((const char**)registers->ecx, (char**)registers->edx, (char*)registers->esi);
//         lock_task_queue();
//         if (!multitasking_add_task_from_vfs((char*)registers->ebx, (char*)registers->ebx, 3, false, &data))
//         {
//             unlock_task_queue();
//             registers->eax = ENOENT;
//             break;
//         }
//         else
//         {
//             uint16_t new_task_index = task_count - 1;
//             pid_t old_pid = __CURRENT_TASK.pid;
//             __CURRENT_TASK.is_dead = __CURRENT_TASK.to_reap = true;
//             tasks[new_task_index].parent = __CURRENT_TASK.parent;
//             __CURRENT_TASK.parent = -1;
//             __CURRENT_TASK.pid = tasks[new_task_index].pid;
//             tasks[new_task_index].pid = old_pid;

//             task_copy_file_table(current_task_index, new_task_index, true);

//             unlock_task_queue();
//             switch_task();
//             break;
//         }
//     }

//     case SYSCALL_WAITPID: // * waitpid | pid = $ebx, options = $edx | $eax = errno, $ebx = *wstatus, $ecx = return_value
//     {
//         pid_t pid = registers->ecx;
//         lock_task_queue();
//         __CURRENT_TASK.wait_pid = registers->ebx;
//         unlock_task_queue();
//         switch_task();
//         registers->ecx = pid;
//         registers->ebx = __CURRENT_TASK.wstatus;
//         break;
//     }

//     case SYSCALL_STAT:  // * stat | path = $ebx, stat_buf = $ecx | $eax = ret   
//     {
//         struct stat* st = (struct stat*)registers->ecx;
//         const char* path = (const char*)registers->ebx;
//         registers->eax = vfs_stat(path, st);
//         break;
//     }

//     case SYSCALL_ACCESS:  // * access | path = $ebx, mode = $ecx | $eax = ret
//     {
//         const char* path = (const char*)registers->ebx;
//         registers->eax = vfs_access(path, registers->ecx);
//         break;
//     }

//     case SYSCALL_READDIR:   // * readdir | &dirent_entry = $ebx, dirp = $ecx | $eax = errno, $ebx = return_address
//     {
//         struct dirent* dirent_entry = (struct dirent*)registers->ebx;
//         DIR* dirp = (DIR*)registers->ecx;
//         registers->ebx = (uint32_t)vfs_readdir(dirent_entry, dirp);
//         registers->eax = errno;
//         break;
//     }

//     case SYSCALL_ISATTY:    // * isatty | fd = $ebx | $eax = errno, $ebx = ret
//     {
//         int fd = *(int*)&registers->ebx;
//         if (fd < 0 || fd >= OPEN_MAX)
//         {
//             registers->eax = EBADF;
//             registers->ebx = 0;
//             break;
//         }
//         if (__CURRENT_TASK.file_table[fd] == invalid_fd)
//         {
//             registers->eax = EBADF;
//             registers->ebx = 0;
//             break;
//         }
//         registers->ebx = __CURRENT_TASK.file_table[fd] < 3;
//         if (!registers->ebx)
//             registers->eax = ENOTTY;
//         break;
//     }

//     case SYSCALL_TCGETATTR:     // * tcgetattr | fildes = $ebx, termios_p = $ecx | $eax = errno
//     {
//         struct termios* termios_p = (struct termios*)registers->ecx;
//         if (!termios_p)
//         {
//             registers->eax = EINVAL;
//             break;
//         }
//         int fd = *(int*)&registers->ebx;
//         if (fd < 0 || fd >= OPEN_MAX)
//         {
//             registers->eax = EBADF;
//             break;
//         }
//         if (__CURRENT_TASK.file_table[fd] == invalid_fd)
//         {
//             registers->eax = EBADF;
//             break;
//         }
//         if (file_table[__CURRENT_TASK.file_table[fd]].type != DT_TERMINAL)  // ! not a tty
//         {
//             registers->eax = ENOTTY;
//             break;
//         }
//         *termios_p = file_table[__CURRENT_TASK.file_table[fd]].data.terminal_data.ts;
//         registers->eax = 0;
//         break;
//     }

//     case SYSCALL_TCSETATTR:     // * tcsetattr | fildes = $ebx, termios_p = $ecx, optional_actions = $edx | $eax = errno
//     {
//         struct termios* termios_p = (struct termios*)registers->ecx;
//         if (!termios_p)
//         {
//             registers->eax = EINVAL;
//             break;
//         }
//         int fd = *(int*)&registers->ebx;
//         if (fd < 0 || fd >= OPEN_MAX)
//         {
//             registers->eax = EBADF;
//             break;
//         }
//         if (__CURRENT_TASK.file_table[fd] == invalid_fd)
//         {
//             registers->eax = EBADF;
//             break;
//         }
//         if (file_table[__CURRENT_TASK.file_table[fd]].type != DT_TERMINAL)  // ! not a tty
//         {
//             registers->eax = ENOTTY;
//             break;
//         }
//         file_table[__CURRENT_TASK.file_table[fd]].data.terminal_data.ts = *termios_p;
//         registers->eax = 0;
//         break;
//     }

//     case SYSCALL_FLUSH_INPUT_BUFFER:
//         utf32_buffer_clear(&(__CURRENT_TASK.input_buffer));
//         break;

//     case SYSCALL_SET_KB_LAYOUT:
//         if (__CURRENT_TASK.ring == 0 && registers->ebx >= 1 && registers->ebx <= NUM_KB_LAYOUTS)
//         {
//             current_keyboard_layout = keyboard_layouts[registers->ebx - 1];
//             registers->eax = 1;
//         }
//         else
//             registers->eax = 0;
//         break;

//     case SYSCALL_BRK_ALLOC: // * brk_alloc | address = $ebx | $eax = num_pages_allocated
//         {
//             // LOG(DEBUG, "alloc 0x%llx", registers->rbx);

//             if (registers->ebx & 0xfff) // ! address not page aligned
//             {
//                 registers->eax = 0;
//                 break;
//             }
            
//             uint32_t pde = read_physical_address_4b(__CURRENT_TASK.cr3 + 4 * (registers->ebx >> 22));
//             physical_address_t pt_address = (physical_address_t)pde & 0xfffff000;
//             if (!(pde & 1))
//             {
//                 pt_address = pfa_allocate_physical_page();
//                 if (pt_address == physical_null)
//                 {
//                     registers->eax = 0;
//                     break;
//                 }
//                 physical_init_page_table(pt_address);
//                 physical_add_page_table(__CURRENT_TASK.cr3, 
//                                         registers->ebx >> 22, 
//                                         pt_address, 
//                                         PAGING_USER_LEVEL, 
//                                         true);
//             }
//             uint32_t pte = read_physical_address_4b(pt_address + 4 * ((registers->ebx >> 12) & 0x3ff));
//             if (!(pte & 1))
//             {
//                 physical_address_t page = pfa_allocate_physical_page();
//                 if (page == physical_null)
//                 {
//                     registers->eax = 0;
//                     break;
//                 }
//                 physical_set_page(  pt_address, 
//                                     ((registers->ebx >> 12) & 0x3ff), 
//                                     page, 
//                                     PAGING_USER_LEVEL, 
//                                     true);
//                 memset_page(page, 0);

//                 #ifdef USE_IVLPG
//                 uint32_t* recursive_paging_pte = (uint32_t*)(((uint32_t)4 * 1024 * 1024 * 1023) | (4 * ((registers->ebx >> 22) * 1024 + ((registers->ebx >> 12) & 0x3ff))));
//                 *recursive_paging_pte = (page & 0xfffff000) | 0b1111;  // * Write-through caching | User level | Read write | Present

//                 invlpg((uint32_t)recursive_paging_pte);
//                 invlpg(4096 * (uint32_t)((registers->ebx >> 22) * 1024 + ((registers->ebx >> 12) & 0x3ff)));
//                 #else
//                 load_pd_by_physaddr(__CURRENT_TASK.cr3);
//                 #endif
                
//                 registers->eax = 1;

//                 // LOG(DEBUG, "Allocated page at address : 0x%llx", registers->rbx);
//             }
//             else
//                 registers->eax = 0;
//         }
//         break;

//     case SYSCALL_BRK_FREE: // * brk_free | address = $ebx | $eax = num_pages_freed
//         {
//             // LOG(DEBUG, "free 0x%llx", registers->rbx);

//             if (registers->ebx & 0xfff)
//             {
//                 registers->eax = 0;
//                 break;
//             }
//             uint32_t pde = read_physical_address_4b(__CURRENT_TASK.cr3 + 4 * (registers->ebx >> 22));
//             if (!(pde & 1))
//             {
//                 registers->eax = 0;
//                 break;
//             }                
//             physical_address_t pt_address = (physical_address_t)pde & 0xfffff000;
//             uint32_t pte = read_physical_address_4b(pt_address + 4 * ((registers->ebx >> 12) & 0x3ff));
//             if (!(pte & 1))
//                 registers->eax = 0;
//             else
//             {
//                 pfa_free_physical_page((physical_address_t)pte & 0xfffff000);
//                 physical_remove_page(pt_address, ((registers->ebx >> 12) & 0x3ff));

//                 #ifdef USE_IVLPG
//                 uint32_t* recursive_paging_pte = (uint32_t*)(((uint32_t)4 * 1024 * 1024 * 1023) | (4 * ((registers->ebx >> 22) * 1024 + ((registers->ebx >> 12) & 0x3ff))));
//                 *recursive_paging_pte = 0b1000;  // * Write-through caching | Not present

//                 invlpg((uint32_t)recursive_paging_pte);
//                 invlpg(4096 * (uint32_t)((registers->ebx >> 22) * 1024 + ((registers->ebx >> 12) & 0x3ff)));
//                 #else
//                 load_pd_by_physaddr(__CURRENT_TASK.cr3);
//                 #endif

//                 registers->eax = 1;
//             }
//         }
//         break;

//     default:
//         LOG(ERROR, "Undefined system call (0x%llx)", registers->eax);
//     // #define DEBUG_SYSCALLS
//     #ifndef DEBUG_SYSCALLS
//         __CURRENT_TASK.is_dead = true;
//         __CURRENT_TASK.return_value = 0x80000000;
//         switch_task();
//     #else
//         abort();
//     #endif
//     }        
//     #ifdef LOG_SYSCALLS
//     LOG(TRACE, "Successfully handled syscall");
//     #endif
// }