void* memset(void* ptr, int value, size_t num)
{
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < num; i++)
        p[i] = value;
    return ptr;
}

void* memcpy(void* destination, const void* source, size_t num)
{
    uint8_t* d = (uint8_t*)destination;
    const uint8_t* s = (const uint8_t*)source;
    for (size_t i = 0; i < num; i++)
        d[i] = s[i];
    return destination;
}

void* memmove(void* dst, const void* src, size_t length)
{
    uint8_t data[length];
    __builtin_memcpy(data, src, length);
    __builtin_memcpy(dst, data, length);
    return dst;
}

int memcmp(const void* str1, const void* str2, size_t n)
{
    for(uint64_t i = 0; i < n; i++)
    {
        if(((uint8_t*)str1)[i] < ((uint8_t*)str2)[i])
            return -1;
        if(((uint8_t*)str1)[i] > ((uint8_t*)str2)[i])
            return 1;
    }

    return 0; 
}

size_t strlen(const char* s)
{
    size_t ret = 0;
    while (s[ret++]);
    return ret - 1;
}

int strcmp(const char* str1, const char* str2)
{
    const unsigned char* p1 = (const unsigned char*)str1;
    const unsigned char* p2 = (const unsigned char*)str2;

    while (*p1 && (*p1 == *p2)) 
    {
        p1++; 
        p2++; 
    }

    return (*p1 > *p2) - (*p2 > *p1);
}

char* strcpy(char* destination, const char* source)
{
    char* _dest = destination;
    while (*source)
    {
        *destination = *source;
        destination++;
        source++;
    }
    (*destination) = 0;
    return _dest;
}

char* strncpy(char* destination, const char* source, size_t n)
{
    char* _dest = destination;
    size_t i = 0;

    while(i < n && source[i] != '\0')
    {
        destination[i] = source[i];
        i++;
    }

    while(i < n)
        destination[i++] = '\0';

    return _dest;
}

char* strerror(int errnum)
{
    switch (errnum) 
    {
        case E2BIG: return "Argument list too long";
        case EACCES: return "Permission denied";
        case EADDRINUSE: return "Address already in use";
        case EADDRNOTAVAIL: return "Address not available";
        case EAFNOSUPPORT: return "Address family not supported";
        case EAGAIN: return "Resource temporarily unavailable";
        case EALREADY: return "Connection already in progress";
        case EBADF: return "Bad file descriptor";
        case EBUSY: return "Device or resource busy";
        case ECHILD: return "No child processes";
        case ECONNABORTED: return "Connection aborted";
        case ECONNREFUSED: return "Connection refused";
        case ECONNRESET: return "Connection reset";
        case EDEADLK: return "Resource deadlock avoided";
        case EDOM: return "Math argument out of domain";
        case EEXIST: return "File exists";
        case EFAULT: return "Bad address";
        case EFBIG: return "File too large";
        case EINTR: return "Interrupted system call";
        case EINVAL: return "Invalid argument";
        case EIO: return "I/O error";
        case EISDIR: return "Is a directory";
        case EMFILE: return "Too many open files";
        case ENOENT: return "No such file or directory";
        case ENOMEM: return "Out of memory";
        case ENOSPC: return "No space left on device";
        case ENOTDIR: return "Not a directory";
        case ENOTEMPTY: return "Directory not empty";
        case ENOTSUP: return "Operation not supported";
        case EPERM: return "Operation not permitted";
        case EPIPE: return "Broken pipe";
        case ERANGE: return "Result too large";
        case EROFS: return "Read-only file system";
        case ESPIPE: return "Illegal seek";
        case ETIMEDOUT: return "Connection timed out";
        case EXDEV: return "Cross-device link";
        default: return "Unknown error";
    }
}