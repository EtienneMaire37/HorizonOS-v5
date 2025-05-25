#pragma once

ssize_t write(int fildes, const void *buf, size_t nbyte)
{
    ssize_t bytes_written;
    asm volatile("int 0xf0" : "=a" (bytes_written), "=b" (errno)
        : "a" (SYSCALL_WRITE), "b" (fildes), "c" (buf), "d" (nbyte));
    return bytes_written;
}

ssize_t read(int fildes, void *buf, size_t nbyte)
{
    ssize_t bytes_read;
    asm volatile("int 0xf0" : "=a" (bytes_read), "=b" (errno)
        : "a" (SYSCALL_READ), "b" (fildes), "c" (buf), "d" (nbyte));
    return bytes_read;
}

void exit(int r)
{
    for (uint8_t i = 0; i < atexit_stack_length; i++)
        if (atexit_stack[atexit_stack_length - i - 1] != NULL)
            atexit_stack[atexit_stack_length - i - 1]();

    asm volatile("int 0xf0" : 
        : "a" (SYSCALL_EXIT), "b" (r));

    while (true);
}

time_t time(time_t* t)
{
    time_t now = 0;
    asm volatile("int 0xf0" : "=a" (now)
        : "a" (SYSCALL_TIME));
    if (t) *t = now;
    return now;
}

pid_t getpid()
{
    uint32_t hi, lo;
    asm volatile("int 0xf0" : "=a" (hi), "=b" (lo)
        : "a" (SYSCALL_GETPID));
    return ((pid_t)hi << 32) | lo;
}

pid_t fork()
{
    uint32_t hi, lo;
    asm volatile("int 0xf0" : "=a" (hi), "=b" (lo)
        : "a" (SYSCALL_FORK));
    uint64_t combined = ((uint64_t)hi << 32) | lo;
    return *(pid_t*)&combined;
}

int brk(void* addr)
{
    if ((uint32_t)addr == break_address) return 0;
    if ((uint32_t)addr < heap_address)
    {
        errno = ENOMEM;
        return -1;
    }
    if ((uint32_t)addr < break_address)
    {
        uint32_t pages_to_free = (alloc_break_address - (uint32_t)addr) / 4096;
        for (uint32_t i = 0; i < pages_to_free; i++)
        {
            uint32_t ret;
            asm volatile("int 0xf0" : "=a" (ret)
                : "a" (SYSCALL_BRK_FREE), "b" ((uint32_t)alloc_break_address - 4096 - 4096 * (uint32_t)i));
            if (ret == 0)
            {
                errno = ENOMEM;
                return -1;
            }
        }
        break_address = (uint32_t)addr;
        alloc_break_address -= 4096 * (uint32_t)pages_to_free;
        alloc_break_address = break_address;
        return 0;
    }

    // ~ (uint32_t)addr > break_address

    uint32_t pages_to_allocate = ((uint32_t)addr - alloc_break_address + 4095) / 4096;
    for (uint32_t i = 0; i < pages_to_allocate; i++)
    {
        uint32_t ret;
        asm volatile("int 0xf0" : "=a" (ret)
            : "a" (SYSCALL_BRK_ALLOC), "b" ((uint32_t)alloc_break_address + 4096 * (uint32_t)i));
        if (ret == 0)
        {
            errno = ENOMEM;
            break_address = (uint32_t)alloc_break_address + 4096 * (uint32_t)i;
            alloc_break_address = break_address;
            return -1;
        }
    }
    break_address = (uint32_t)addr;
    alloc_break_address += 4096 * (uint32_t)pages_to_allocate;
    return 0;
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