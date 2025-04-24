#pragma once

ssize_t write(int fildes, const void *buf, size_t nbyte)
{
    ssize_t bytes_written;
    asm volatile("int 0xff" : "=a" (bytes_written), "=b" (errno)
        : "a" (SYSCALL_WRITE), "b" (fildes), "c" (buf), "d" (nbyte));
    return bytes_written;
}

ssize_t read(int fildes, void *buf, size_t nbyte)
{
    ssize_t bytes_read;
    asm volatile("int 0xff" : "=a" (bytes_read), "=b" (errno)
        : "a" (SYSCALL_READ), "b" (fildes), "c" (buf), "d" (nbyte));
    return bytes_read;
}

void exit(int r)
{
    for (uint8_t i = 0; i < atexit_stack_length; i++)
        if (atexit_stack[atexit_stack_length - i - 1] != NULL)
            atexit_stack[atexit_stack_length - i - 1]();

    asm volatile("int 0xff" : 
        : "a" (SYSCALL_EXIT), "b" (r));

    while (true);
}

time_t time(time_t* t)
{
    time_t now = 0;
    asm volatile("int 0xff" : "=a" (now)
        : "a" (SYSCALL_TIME));
    if (t) *t = now;
    return now;
}

pid_t getpid()
{
    uint32_t hi, lo;
    asm volatile("int 0xff" : "=a" (hi), "=b" (lo)
        : "a" (SYSCALL_GETPID));
    return ((pid_t)hi << 32) | lo;
}

pid_t fork()
{
    uint32_t hi, lo;
    asm volatile("int 0xff" : "=a" (hi), "=b" (lo)
        : "a" (SYSCALL_FORK));
    uint64_t combined = ((uint64_t)hi << 32) | lo;
    return *(pid_t*)&combined;
}