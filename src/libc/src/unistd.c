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

int chdir(const char* path)
{
    if (!path) 
    {
        errno = EFAULT;
        return -1;
    }

    char* new_cwd = realpath(path, NULL);
    if (!new_cwd)
        return -1;

    struct stat st;
    if (stat(new_cwd, &st) == 0) 
    {
        if (S_ISDIR(st.st_mode))
        {
            if (access(new_cwd, F_OK | X_OK) == 0) 
            {
                strncpy(cwd, new_cwd, PATH_MAX - 1);
                cwd[PATH_MAX - 1] = '\0';
                free(new_cwd);
                return 0;
            }
        }
        else
            errno = ENOTDIR;
    }

    int err = errno;
    free(new_cwd);
    errno = err;
    return -1;
}


char* getcwd(char* buffer, size_t size)
{
    if (!buffer || size <= 0) return NULL;
    memcpy(buffer, &cwd[0], minint(size, PATH_MAX) - 1);
    buffer[size - 1] = 0;
    return buffer;
}

int execv(const char* path, char* const argv[])
{
    return execve(path, argv, environ);
}

int execvpe(const char* file, char* const argv[], char* const envp[])
{
#include "misc.h"
    execve(file, argv, envp);

    const size_t file_len = strlen(file);
    for (size_t i = 0; i < file_len; i++)
    {
        if (file[i] == '/') 
        {
            errno = ENOENT;
            return -1;
        }
    }

    const char* PATH = getenv("PATH");
    if (!PATH) 
    {
        errno = ENOENT;
        return -1;
    }
    int bytes = strlen(PATH) + 1;
    char* path_data = malloc(bytes);
    strncpy(path_data, PATH, bytes);
    for (int i = 0; i < bytes; i++)
    {
        if (path_data[i] == ':')
            path_data[i] = 0;
    }
    char* path = path_data;
    do
    {
        int path_len = strlen(path);
        char* combined = malloc(path_len + file_len + 2);
        strcpy(combined, path);
        combined[path_len] = '/';
        strcpy(((void*)combined + path_len + 1), file);
        execve(combined, argv, envp);
        free(combined);
    }
    while ((path = find_next_contiguous_string(path, &bytes)));
    free(path_data);
    errno = ENOENT;
    return -1;
}

int execvp(const char* file, char* const argv[])
{
    return execvpe(file, argv, environ);
}