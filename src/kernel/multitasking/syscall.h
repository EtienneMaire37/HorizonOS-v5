#pragma once

#include "../int/int.h"
#include "../../libc/src/syscall_defines.h"
#include "../../libc/src/startup_data.h"
#include "../multitasking/loader.h"

void handle_syscall(interrupt_registers_t* registers)
{
    switch (registers->rax)     // !! some of the path resolution is handled in libc
    {                           // TODO: Implement a hierarchichal VFS
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
                registers->rax = minint(get_buffered_characters(__CURRENT_TASK.input_buffer), registers->rdx);
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

    case SYSCALL_EXECVE:    // * execve | path = $rbx, argv = $rcx, envp = $rdx, cwd = $rsi | $rax = errno
    {
        if (vfs_access((char*)registers->rbx, X_OK) != 0)
        {
            registers->rax = EACCES;
            break;
        }
        startup_data_struct_t data = startup_data_init_from_command((char**)registers->rcx, (char**)registers->rdx, (char*)registers->rsi);
        lock_task_queue();
        if (!multitasking_add_task_from_vfs((char*)registers->rbx, (char*)registers->rbx, 3, false, &data))
        {
            unlock_task_queue();
            registers->rax = ENOENT;
            break;
        }
        else
        {
            uint16_t new_task_index = task_count - 1;
            pid_t old_pid = __CURRENT_TASK.pid;
            __CURRENT_TASK.is_dead = __CURRENT_TASK.to_reap = true;
            tasks[new_task_index].parent = __CURRENT_TASK.parent;
            __CURRENT_TASK.parent = -1;
            __CURRENT_TASK.pid = tasks[new_task_index].pid;
            tasks[new_task_index].pid = old_pid;

            task_copy_file_table(current_task_index, new_task_index, true);

            unlock_task_queue();
            switch_task();
            break;
        }
    }

    case SYSCALL_WAITPID: // * waitpid | pid = $rbx, options = $rdx | $rax = errno, $rbx = *wstatus, $rcx = return_value
    {
        pid_t pid = registers->rcx;
        lock_task_queue();
        __CURRENT_TASK.wait_pid = registers->rbx;
        unlock_task_queue();
        switch_task();
        registers->rcx = pid;
        registers->rbx = __CURRENT_TASK.wstatus;
        break;
    }

    case SYSCALL_GETPID:     // * getpid || $rax = pid
        lock_task_queue();
        registers->rax = __CURRENT_TASK.pid;
        unlock_task_queue();
        break;

    case SYSCALL_FORK:     // * fork
        if (task_count >= MAX_TASKS)
        {
            registers->rax = (uint64_t)-1;
        }
        else
        {
            lock_task_queue();
            __CURRENT_TASK.forked_pid = current_pid++;
            pid_t forked_pid = __CURRENT_TASK.forked_pid;
            unlock_task_queue();
            switch_task();
            if (__CURRENT_TASK.pid == forked_pid)
                registers->rax = 0;
            else
            {
                registers->rax = forked_pid;
            }
        } 
        break;

    case SYSCALL_TCGETATTR:     // * tcgetattr | fildes = $rbx, termios_p = $rcx | $rax = errno
    {
        struct termios* termios_p = (struct termios*)registers->rcx;
        if (!termios_p)
        {
            registers->rax = EINVAL;
            break;
        }
        int fd = (int)registers->rbx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->rax = EBADF;
            break;
        }
        if (__CURRENT_TASK.file_table[fd] == invalid_fd)
        {
            registers->rax = EBADF;
            break;
        }
        if (file_table[__CURRENT_TASK.file_table[fd]].type != DT_TERMINAL)  // ! not a tty
        {
            registers->rax = ENOTTY;
            break;
        }
        *termios_p = file_table[__CURRENT_TASK.file_table[fd]].data.terminal_data.ts;
        registers->rax = 0;
        break;
    }

    case SYSCALL_TCSETATTR:     // * tcsetattr | fildes = $rbx, termios_p = $rcx, optional_actions = $rdx | $rax = errno
    {
        struct termios* termios_p = (struct termios*)registers->rcx;
        if (!termios_p)
        {
            registers->rax = EINVAL;
            break;
        }
        int fd = (int)registers->rbx;
        if (fd < 0 || fd >= OPEN_MAX)
        {
            registers->rax = EBADF;
            break;
        }
        if (__CURRENT_TASK.file_table[fd] == invalid_fd)
        {
            registers->rax = EBADF;
            break;
        }
        if (file_table[__CURRENT_TASK.file_table[fd]].type != DT_TERMINAL)  // ! not a tty
        {
            registers->rax = ENOTTY;
            break;
        }
        file_table[__CURRENT_TASK.file_table[fd]].data.terminal_data.ts = *termios_p;
        registers->rax = 0;
        break;
    }

    case SYSCALL_SET_KB_LAYOUT:
    // * Should probably apply some form of security (user system)
        if (registers->rbx >= 1 && registers->rbx <= NUM_KB_LAYOUTS)
        {
            current_keyboard_layout = keyboard_layouts[registers->rbx - 1];
            registers->rax = 1;
        }
        else
            registers->rax = 0;
        break;

    case SYSCALL_FLUSH_INPUT_BUFFER:
        utf32_buffer_clear(&(__CURRENT_TASK.input_buffer));
        break;

    case SYSCALL_STAT:  // * stat | path = $rbx, stat_buf = $rcx | $rax = ret   
    {
        struct stat* st = (struct stat*)registers->rcx;
        const char* path = (const char*)registers->rbx;
        registers->rax = vfs_stat(path, st);
        break;
    }

    case SYSCALL_ACCESS:  // * access | path = $rbx, mode = $rcx | $rax = ret
    {
        const char* path = (const char*)registers->rbx;
        registers->rax = vfs_access(path, registers->rcx);
        break;
    }

    case SYSCALL_READDIR:   // * readdir | &dirent_entry = $rbx, dirp = $rcx | $rax = errno, $rbx = return_address
    {
        struct dirent* dirent_entry = (struct dirent*)registers->rbx;
        DIR* dirp = (DIR*)registers->rcx;
        registers->rbx = (uintptr_t)vfs_readdir(dirent_entry, dirp);
        registers->rax = errno;
        break;
    }

    default:
        LOG(ERROR, "Undefined system call (%#llx)", registers->rax);
        
        __CURRENT_TASK.is_dead = true;
        __CURRENT_TASK.return_value = 0x80000000;
        switch_task();
    }
}