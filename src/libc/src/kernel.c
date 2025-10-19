#pragma once

ssize_t write(int fildes, const void* buf, size_t nbyte)
{
    // abort();

    if (fildes > 2)
    {
        LOG(ERROR, "Invalid output stream");
        errno = EBADF;
        return -1;
    }

    switch(fildes)
    {
    case STDIN_FILENO: 
        return 0;
    case STDERR_FILENO: // * Kernel writes to stderr -> log
        for (size_t i = 0; i < nbyte; i++)
            debug_outc(*((char*)buf + i));
        return nbyte;
    case STDOUT_FILENO:
        for (size_t i = 0; i < nbyte; i++)
            tty_outc(*((char*)buf + i));
        tty_update_cursor();
        return nbyte;
    }

    return 0;
}

ssize_t read(int fildes, void* buf, size_t nbyte)
{
    abort();

    if (fildes > 2)
    {
        LOG(ERROR, "Invalid input stream");
        errno = EBADF;
        return -1;
    }

    if (nbyte == 0) return 0;

    switch(fildes)
    {
    case STDIN_FILENO:
        return 0;
    case STDERR_FILENO:
        return 0;
    case STDOUT_FILENO:
        return 0;
    }

    return 0;
}

void exit(int r)
{
    disable_interrupts();
    LOG(CRITICAL, "Kernel exited with return code %d", r);
    tty_set_color(FG_WHITE, BG_BLACK);
    printf("\nKernel exited with return code ");
    tty_set_color(FG_LIGHTGREEN, BG_BLACK);
    printf("%d", r);
    tty_set_color(FG_WHITE, BG_BLACK);
    printf(".");
    fflush(stdout);
    halt();
    while (true);
    __builtin_unreachable();
}

time_t time(time_t* t)
{
    abort();
    return ktime(t);
}

pid_t getpid()
{
    abort();
    return (pid_t)-1;
}

pid_t fork()
{
    abort();
    errno = ENOMEM; // ...
    return (pid_t)-1;
}

int brk(void*addr)
{
    abort();
    errno = ENOMEM;
    return -1;
}

int open(const char* path, int oflag, ...)
{
    abort();
    errno = ENOENT;
    return -1;
}

int close(int fildes)
{
    abort();
    errno = EBADF;
    return -1;
}

void* malloc(size_t bytes)
{
    abort();
    errno = ENOMEM;
    return NULL;
}

void free(void* ptr)
{
    abort();
}

int execve(const char* path, char* const argv[], char* const envp[])
{
    abort();
    errno = EACCES;
    return -1;
}

pid_t waitpid(pid_t pid, int* wstatus, int options)
{
    abort();
    while(true);
}

int access(const char* path, int mode)
{
    // if (!mode) return 0;
    // return vfs_path_exists(path);
    abort();
    return 0;
}

int stat(const char* path, struct stat* statbuf)
{
    abort();
    return 0;
}

int isatty(int fd)
{
    abort();
    return 0;
}

int tcgetattr(int fildes, struct termios* termios_p)
{
    abort();
}

int tcsetattr(int fildes, int optional_actions, const struct termios* termios_p)
{
    abort();
}