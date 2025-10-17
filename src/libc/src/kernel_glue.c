#pragma once

ssize_t write(int fildes, const void *buf, size_t nbyte)
{
    ssize_t bytes_written;
    int _errno;
    asm volatile("int 0xf0" : "=a" (bytes_written), "=b" (_errno)
        : "a" (SYSCALL_WRITE), "b" (fildes), "c" (buf), "d" (nbyte));
    if (_errno) errno = _errno;
    return bytes_written;
}

ssize_t read(int fildes, void *buf, size_t nbyte)
{
    ssize_t bytes_read;
    int _errno;
    asm volatile("int 0xf0" : "=a" (bytes_read), "=b" (_errno)
        : "a" (SYSCALL_READ), "b" (fildes), "c" (buf), "d" (nbyte) : "memory");
    if (_errno) errno = _errno;
    return bytes_read;
}

void exit(int r)
{
    fflush(stdout);

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
    pid_t ret = (pid_t)combined;
    return ret;
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
        uint32_t aligned_target = ((uint32_t)addr + 4095) & ~0xfff;
        uint32_t pages_to_free = (alloc_break_address - aligned_target) / 4096;

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
        // alloc_break_address = break_address;
        return 0;
    }

    // ~ (uint32_t)addr > break_address

    uint32_t aligned_alloc_break = (alloc_break_address + 4095) & ~4095;
    uint32_t aligned_target = ((uint32_t)addr + 4095) & ~4095;
    uint32_t pages_to_allocate = (aligned_target - aligned_alloc_break) / 4096;

    for (uint32_t i = 0; i < pages_to_allocate; i++)
    {
        uint32_t ret;
        asm volatile("int 0xf0" : "=a" (ret)
            : "a" (SYSCALL_BRK_ALLOC), "b" ((uint32_t)alloc_break_address + 4096 * (uint32_t)i));
        if (ret == 0)
        {
            errno = ENOMEM;
            break_address = (uint32_t)alloc_break_address + 4096 * (uint32_t)i;
            // alloc_break_address = break_address;
            return -1;
        }
    }
    break_address = (uint32_t)addr;
    alloc_break_address += 4096 * (uint32_t)pages_to_allocate;
    return 0;
}

int open(const char* path, int oflag, ...)
{
    mode_t mode = 0;
    if ((oflag & O_CREAT) || (oflag & O_TMPFILE))
    {
        va_list args;
        va_start(args, oflag);
        mode = va_arg(args, mode_t) & ~fd_creation_mask;
        va_end(args);
    }
    char* _path = realpath(path, NULL);
    int fd, _errno;
    asm volatile ("int 0xf0" : "=a"(_errno), "=b"(fd) : "a"(SYSCALL_OPEN), "b"(_path), "c"(oflag), "d"(mode));
    if (_errno != 0)
        errno = _errno;
    free(_path);
    return fd;
}

int close(int fildes)
{
    int ret, _errno;
    asm volatile ("int 0xf0" : "=a"(_errno), "=b"(ret) : "a"(SYSCALL_CLOSE), "b"(fildes));
    if (_errno != 0)
        errno = _errno;
    return ret;
}

int execve(const char* path, char* const argv[], char* const envp[])
{
    char resolved_path[PATH_MAX] = {0};
    realpath(path, resolved_path);
    int _errno;
    asm volatile ("int 0xf0" : "=a"(_errno) : "a"(SYSCALL_EXECVE), "b"(resolved_path), "c"(argv), "d"(envp), "S"(cwd));
    if (_errno != 0)
        errno = _errno;
    return -1;
}

pid_t waitpid(pid_t pid, int* wstatus, int options)
{
    int _wstatus, _errno;
    uint32_t ret_lo, ret_hi;
    uint32_t pid_lo = pid & 0xffffffff, pid_hi = pid >> 32;
    asm volatile ("int 0xf0" : "=a"(_errno), "=b"(_wstatus), "=c"(ret_lo), "=d"(ret_hi) : "a"(SYSCALL_WAITPID), "b"(pid_lo), "c"(pid_hi), "d"(options) : "memory");
    if (wstatus) *wstatus = _wstatus;
    if (_errno != 0)
        errno = _errno;
    uint64_t ret = ((uint64_t)ret_hi << 32) | ret_lo;
    return *(pid_t*)&ret;
}

int access(const char* path, int mode)
{
    char* _path = realpath(path, NULL);
    int ret;
    asm volatile ("int 0xf0" : "=a"(ret) : "a"(SYSCALL_ACCESS), "b"(_path), "c"(mode));
    if (ret != 0)
    {
        free(_path);
        errno = ret;
        return -1;
    }
    free(_path);
    return 0;
}

int stat(const char* path, struct stat* statbuf)
{
    char* _path = realpath(path, NULL);
    int ret;
    asm volatile ("int 0xf0" : "=a"(ret) : "a"(SYSCALL_STAT), "b"(_path), "c"((uint32_t)statbuf) : "memory");
    if (ret != 0)
    {
        free(_path);
        errno = ret;
        return -1;
    }
    free(_path);
    return 0;
}

static struct dirent dirent_entry;      // * "This structure may be statically allocated"

struct dirent* readdir(DIR* dirp)
{
    if (dirp->current_path[0])
    {
        char current_path[PATH_MAX];
        memcpy(current_path, dirp->current_path, PATH_MAX);
        int i = 0, j = 0;
        bool slash = 1;
        while (current_path[j - 1 + slash])
        {
            if (dirp->path[i])
            {
                dirp->current_path[i + j] = dirp->path[i];
                i++;
            }
            else
            {
                if (slash)
                {
                    dirp->current_path[i + j] = '/';
                    slash = 0;
                    j++;
                }
                else
                {
                    dirp->current_path[i + j] = current_path[j - 1];
                    j++;
                }
            }
        }
    }
    uint32_t return_address;
    int _errno;
    asm volatile ("int 0xf0" : "=a"(_errno), "=b"(return_address) : "a"(SYSCALL_READDIR), "b"((uint32_t)&dirent_entry), "c"((uint32_t)dirp) : "memory");
    if (_errno != 0)
        errno = _errno;
    return (struct dirent*)return_address;
}

int isatty(int fd)
{
    int ret, _errno;
    asm volatile ("int 0xf0" : "=a"(_errno), "=b"(ret) : "a"(SYSCALL_ISATTY), "b"(fd));
    if (_errno != 0)
        errno = _errno;
    return ret;
}