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
    uint8_t* s = (uint8_t*)source;
    for (size_t i = 0; i < num; i++)
        d[i] = s[i];
    return destination;
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

int strcmp(const char* str1, const char* str2)
{
    const unsigned char *p1 = (const unsigned char *)str1;
    const unsigned char *p2 = (const unsigned char *)str2;

    while (*p1 && (*p1 == *p2)) 
    {
        p1++; 
        p2++; 
    }

    return (*p1 > *p2) - (*p2 > *p1);
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while (*str)
    {
        str++;
        len++;
    }
    return len;
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
    return _dest;
}

char internal_error_message[64];

char* errno_messages[] = 
{
    [0]  = "Success",
    [1]  = "Argument list too long",
    [2]  = "Permission denied",
    [3]  = "Address already in use",
    [4]  = "Address not available",
    [5]  = "Address family not supported",
    [6]  = "Resource temporarily unavailable",
    [7]  = "Operation already in progress",
    [8]  = "Bad file descriptor",
    [9]  = "Bad message",
    [10] = "Device or resource busy",
    [11] = "Operation canceled",
    [12] = "No child processes",
    [13] = "Connection aborted",
    [14] = "Connection refused",
    [15] = "Connection reset by peer",
    [16] = "Resource deadlock avoided",
    [17] = "Destination address required",
    [18] = "Numerical argument out of domain",
    [19] = "Disk quota exceeded",
    [20] = "File exists",
    [21] = "Bad address",
    [22] = "File too large",
    [23] = "Host is unreachable",
    [24] = "Identifier removed",
    [25] = "Illegal byte sequence",
    [26] = "Operation in progress",
    [27] = "Interrupted system call",
    [28] = "Invalid argument",
    [29] = "Input/output error",
    [30] = "Socket is already connected",
    [31] = "Is a directory",
    [32] = "Too many levels of symbolic links",
    [33] = "Too many open files",
    [34] = "Too many links",
    [35] = "Message too long",
    [36] = "Multihop attempted",
    [37] = "File name too long",
    [38] = "Network is down",
    [39] = "Connection aborted by network",
    [40] = "Network unreachable",
    [41] = "Too many open files in system",
    [42] = "No buffer space available",
    [43] = "No data available",
    [44] = "No such device",
    [45] = "No such file or directory",
    [46] = "Exec format error",
    [47] = "No locks available",
    [48] = "Link has been severed",
    [49] = "Out of memory",
    [50] = "No message of desired type",
    [51] = "Protocol not available",
    [52] = "No space left on device",
    [53] = "Out of streams resources",
    [54] = "Not a stream",
    [55] = "Function not implemented",
    [56] = "The socket is not connected",
    [57] = "Not a directory",
    [58] = "Directory not empty",
    [59] = "State not recoverable",
    [60] = "Not a socket",
    [61] = "Operation not supported",
    [62] = "Inappropriate ioctl for device",
    [63] = "No such device or address",
    [64] = "Operation not supported on socket",
    [65] = "Value too large for defined data type",
    [66] = "Owner dead",
    [67] = "Operation not permitted",
    [68] = "Broken pipe",
    [69] = "Protocol error",
    [70] = "Protocol not supported",
    [71] = "Protocol wrong type for socket",
    [72] = "Numerical result out of range",
    [73] = "Read-only file system",
    [74] = "Illegal seek",
    [75] = "No such process",
    [76] = "Stale file handle",
    [77] = "Timer expired",
    [78] = "Connection timed out",
    [79] = "Text file busy",
    [80] = "Operation would block",
    [81] = "Invalid cross-device link"
};

char* strerror(int errnum)
{
    strcpy(&internal_error_message[0], errno_messages[errnum]);
    return &internal_error_message[0];
}