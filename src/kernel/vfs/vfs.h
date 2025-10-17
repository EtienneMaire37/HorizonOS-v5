#pragma once

typedef int16_t file_table_index_t;
const file_table_index_t invalid_fd = -1;

typedef enum drive_type
{
    DT_INITRD,
    DT_INVALID
} drive_type_t;

typedef struct initrd_file_entry_data
{
    initrd_file_t* file;
} initrd_file_entry_data_t;

typedef struct file_entry
{
    int used;
    drive_type_t type;
    int flags;
    uint64_t position;
    union 
    {
        initrd_file_entry_data_t initrd_data;
    } data;
} file_entry_t;

#define MAX_FILE_TABLE_ENTRIES  256

file_entry_t file_table[MAX_FILE_TABLE_ENTRIES];

atomic_flag file_table_spinlock = ATOMIC_FLAG_INIT;

drive_type_t get_drive_type(const char* path)
{
    const char* initrd_prefix = "/initrd/";
    const char* short_initrd_prefix = "/initrd";

    if (strcmp(short_initrd_prefix, path) == 0)
        return DT_INITRD;

    size_t i = 0;
    while (path[i] != 0 && initrd_prefix[i] != 0 && path[i] == initrd_prefix[i])
        i++;
    const size_t len = strlen(initrd_prefix);
    if (i == len)
        return DT_INITRD;

    return DT_INVALID;
}

void vfs_init_file_table()
{
    acquire_spinlock(&file_table_spinlock);
    for (int i = 0; i < MAX_FILE_TABLE_ENTRIES; i++)
    {
        if (i < 3)  file_table[i].used = 1; // * Will "always" be positive
        else        file_table[i].used = 0;
    }
    release_spinlock(&file_table_spinlock);
}

int vfs_allocate_global_file()
{
    acquire_spinlock(&file_table_spinlock);
    for (int i = 3; i < MAX_FILE_TABLE_ENTRIES; i++)
    {
        if (file_table[i].used == 0)
        {
            file_table[i].used = 1;
            release_spinlock(&file_table_spinlock);
            return i;
        }
    }
    release_spinlock(&file_table_spinlock);
    return -1;
}

void vfs_remove_global_file(int fd)
{
    acquire_spinlock(&file_table_spinlock);
    if (fd >= 3 && fd < MAX_FILE_TABLE_ENTRIES)
    {
        file_table[fd].used--;
        if (file_table[fd].used <= 0)
        {
            file_table[fd].used = 0;
            file_table[fd].type = DT_INVALID;
        }
        release_spinlock(&file_table_spinlock);
        return;
    }
    release_spinlock(&file_table_spinlock);
}

int vfs_root_stat(struct stat* st);
int vfs_stat(const char* path, struct stat* st);
int vfs_access(const char* path, mode_t mode);
struct dirent* vfs_readdir(struct dirent* dirent, DIR* dirp);