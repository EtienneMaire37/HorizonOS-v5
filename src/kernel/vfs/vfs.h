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
    union 
    {
        initrd_file_entry_data_t initrd_data;
    } data;
    
} file_entry_t;

file_entry_t file_table[256];

void vfs_init_file_table()
{
    for (int i = 0; i < 256; i++)
    {
        if (i < 3)  file_table[i].used = 1; // * Will "always" be positive
        else        file_table[i].used = 0;
    }
}

int vfs_root_stat(struct stat* st);
int vfs_stat(const char* path, struct stat* st);
int vfs_access(const char* path, mode_t mode);
struct dirent* vfs_readdir(struct dirent* dirent, DIR* dirp);