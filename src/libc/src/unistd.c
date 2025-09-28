uint32_t break_address, heap_address, alloc_break_address;
uint32_t heap_size;

void* sbrk(intptr_t incr)
{
    void* old_break = (void*)break_address;
    if (brk((void*)((uint32_t)(break_address + incr))) != 0)
        return (void*)-1;
    return old_break;
}

off_t lseek(int fildes, off_t offset, int whence)
{
// ! For now we assume only standard streams
    errno = ESPIPE;
    return -1;
}

static const char* host_name = "horizonos-pc";

int gethostname(char* name, size_t namelen)
{
    if (!name || namelen <= 0) return -1;
    memcpy(name, host_name, namelen - 1);
    name[namelen - 1] = 0;

    return 0;
}

int chdir(const char* path) // !!! unstub this asap
{
    // char old_cwd[PATH_MAX];
    // getcwd(old_cwd, PATH_MAX);
    // path_add(cwd, cwd, path);
    if (path == NULL)
    {
        errno = EFAULT;
        return -1;
    }
    errno = EACCES;
    return -1;
}

char* getcwd(char* buffer, size_t size)
{
    if (!buffer || size <= 0) return NULL;
    memcpy(buffer, &cwd[0], minint(size, PATH_MAX) - 1);
    buffer[size - 1] = 0;
    return buffer;
}