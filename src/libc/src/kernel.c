#pragma once

ssize_t write(int fildes, const void *buf, size_t nbyte)
{
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
    case STDERR_FILENO: // Kernel writes to stderr -> log
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

ssize_t read(int fildes, void *buf, size_t nbyte)
{
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
    LOG(CRITICAL, "Kernel aborted");
    tty_set_color(FG_WHITE, BG_BLACK);
    printf("\nKernel aborted.");
    fflush(stdout);
    halt();
}

time_t time(time_t* t)
{
    return ktime(t);
}

pid_t getpid()
{
    return (pid_t)-1;
}

pid_t fork()
{
    errno = ENOMEM; // ...
    return (pid_t)-1;
}

int brk(void *addr)
{
    errno = ENOMEM;
    return -1;
}

int open(const char* path, int oflag, ...)
{
    errno = ENOENT;
    return -1;
}

int close(int fildes)
{
    errno = EBADF;
    return -1;
}

void* malloc(size_t bytes)
{
    errno = ENOMEM;
    return NULL;
}

void free(void* ptr)
{
    ;
}