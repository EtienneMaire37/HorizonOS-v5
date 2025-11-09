#pragma once

ssize_t write(int fildes, const void* buf, size_t nbyte)
{
    ssize_t bytes_written;
    int _errno;
    asm volatile("int 0xf0" : "=a" (bytes_written), "=b" (_errno)
        : "a" (SYSCALL_WRITE), "b" (fildes), "c" (buf), "d" (nbyte));
    if (_errno) errno = _errno;
    return bytes_written;
}

ssize_t read(int fildes, void* buf, size_t nbyte)
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
    __builtin_unreachable();
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
    pid_t ret;
    asm volatile("int 0xf0" : "=a" (ret) : "a" (SYSCALL_GETPID));
    return ret;
}

pid_t fork()
{
    pid_t ret;
    asm volatile("int 0xf0" : "=a" (ret) : "a" (SYSCALL_FORK));
    return ret;
}

int brk(void* addr)
{
    if ((uint64_t)addr == break_address) return 0;
    if ((uint64_t)addr < heap_address)
    {
        errno = ENOMEM;
        return -1;
    }
    if ((uint64_t)addr < break_address)
    {
        uint64_t aligned_target = ((uint64_t)addr + 0xfff) & ~0xfffULL;
        uint64_t pages_to_free = (alloc_break_address - aligned_target) / 0x1000;

        for (uint64_t i = 0; i < pages_to_free; i++)
        {
            uint64_t ret;
            asm volatile("int 0xf0" : "=a" (ret)
                : "a" (SYSCALL_BRK_FREE), "b" ((uint64_t)alloc_break_address - 0x1000 - 0x1000 * i));
            if (ret == 0)
            {
                errno = ENOMEM;
                return -1;
            }
        }
        break_address = (uint64_t)addr;
        alloc_break_address -= 0x1000 * (uint64_t)pages_to_free;
        // alloc_break_address = break_address;
        return 0;
    }

    // ~ (uint32_t)addr > break_address

    uint64_t aligned_alloc_break = (alloc_break_address + 0xfff) & ~0xfffULL;
    uint64_t aligned_target = ((uint64_t)addr + 0xfff) & ~0xfffULL;
    uint64_t pages_to_allocate = (aligned_target - aligned_alloc_break) / 0x1000;

    for (uint64_t i = 0; i < pages_to_allocate; i++)
    {
        uint64_t ret;
        asm volatile("int 0xf0" : "=a" (ret)
            : "a" (SYSCALL_BRK_ALLOC), "b" ((uint64_t)alloc_break_address + 0x1000 * i));
        if (ret == 0)
        {
            errno = ENOMEM;
            break_address = (uint64_t)alloc_break_address + 0x1000 * i;
            // alloc_break_address = break_address;
            return -1;
        }
    }
    break_address = (uint64_t)addr;
    alloc_break_address += 0x1000 * (uint64_t)pages_to_allocate;
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
    pid_t ret;
    asm volatile ("int 0xf0" : "=a"(_errno), "=b"(_wstatus), "=c"(ret) : "a"(SYSCALL_WAITPID), "b"(pid), "d"(options) : "memory");
    if (wstatus) *wstatus = _wstatus;
    if (_errno != 0)
        errno = _errno;
    return ret;
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
    asm volatile ("int 0xf0" : "=a"(ret) : "a"(SYSCALL_STAT), "b"(_path), "c"((uint64_t)statbuf) : "memory");
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
        __builtin_memcpy(current_path, dirp->current_path, PATH_MAX);
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
    uint64_t return_address;
    int _errno;
    asm volatile ("int 0xf0" : "=a"(_errno), "=b"(return_address) : "a"(SYSCALL_READDIR), "b"((uint64_t)&dirent_entry), "c"((uint64_t)dirp) : "memory");
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

int tcgetattr(int fildes, struct termios* termios_p)
{
    int ret;
    asm volatile ("int 0xf0" : "=a"(ret) : "a"(SYSCALL_TCGETATTR), "b"(fildes), "c"(termios_p));
    if (ret)
    {
        errno = ret;
        return -1;
    }
    return 0;
}

int tcsetattr(int fildes, int optional_actions, const struct termios* termios_p)
{
    int ret;
    asm volatile ("int 0xf0" : "=a"(ret) : "a"(SYSCALL_TCSETATTR), "b"(fildes), "c"(termios_p), "d"(optional_actions));
    if (ret)
    {
        errno = ret;
        return -1;
    }
    return 0;
}