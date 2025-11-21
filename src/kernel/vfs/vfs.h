#pragma once

typedef int16_t file_table_index_t;
const file_table_index_t invalid_fd = -1;

typedef enum drive_type
{
    DT_INITRD,
    DT_TERMINAL,
    DT_INVALID
} drive_type_t;

typedef struct initrd_file_entry_data
{
    initrd_file_t* file;
} initrd_file_entry_data_t;

typedef struct terminal_data
{
    struct termios ts;
} terminal_data_t;

typedef struct file_entry
{
    int used;
    drive_type_t type;
    int flags;
    uint64_t position;
    union 
    {
        initrd_file_entry_data_t initrd_data;
        terminal_data_t terminal_data;
    } data;
} file_entry_t;

// * VFS data

#define VFS_NODE_INIT       0

#define VFS_NODE_EXPLORED   1
#define VFS_NODE_LOADING    2

typedef struct vfs_file_tnode vfs_file_tnode_t;
typedef struct vfs_folder_tnode vfs_folder_tnode_t;

// * i-nodes
typedef struct 
{
    struct stat st;

    vfs_folder_tnode_t* parent;
} vfs_file_inode_t;

typedef struct
{
    vfs_file_tnode_t* files;
    vfs_folder_tnode_t* folders;
    ino_t inode_number;

    uint8_t flags;

    vfs_folder_tnode_t* parent;
} vfs_folder_inode_t;

// * t-nodes
typedef struct vfs_file_tnode
{
    char* name;
    vfs_file_inode_t* inode;

    struct vfs_file_tnode* next;
} vfs_file_tnode_t;

typedef struct vfs_folder_tnode
{
    char* name;
    vfs_folder_inode_t* inode;

    struct vfs_folder_tnode* next;
} vfs_folder_tnode_t;

vfs_folder_inode_t* vfs_root = NULL;

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
        if (i < 3)  file_table[i].used = 1; // * Will always be used (hopefully)
        else        file_table[i].used = 0;
    }
    
    file_table[STDIN_FILENO].type = DT_TERMINAL;
    file_table[STDIN_FILENO].data.terminal_data.ts = (struct termios)
    {
        .c_iflag = ICRNL | IXON,
        .c_oflag = OPOST | ONLCR,
        .c_cflag = B38400 | CS8 | CREAD | HUPCL,
        .c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN,
        .c_cc = 
        {
            [VINTR]    = 0x03,
            [VQUIT]    = 0x1C,
            [VERASE]   = 0x7F,
            [VKILL]    = 0x15,
            [VEOF]     = 0x04,
            [VTIME]    = 0,
            [VMIN]     = 1,
            [VSTART]   = 0x11,
            [VSTOP]    = 0x13,
            [VSUSP]    = 0x1A,
            [VEOL]     = 0,
            [VREPRINT] = 0x12,
            [VDISCARD] = 0x0F,
            [VWERASE]  = 0x17,
            [VLNEXT]   = 0x16,
            [VEOL2]    = 0
        }
    };
    file_table[STDIN_FILENO].position = 0;
    file_table[STDIN_FILENO].flags = O_RDONLY;

    file_table[STDOUT_FILENO].type = DT_TERMINAL;
    file_table[STDOUT_FILENO].data.terminal_data.ts = file_table[STDIN_FILENO].data.terminal_data.ts;
    file_table[STDOUT_FILENO].position = 0;
    file_table[STDOUT_FILENO].flags = O_WRONLY;

    file_table[STDERR_FILENO].type = DT_TERMINAL;
    file_table[STDERR_FILENO].data.terminal_data.ts = file_table[STDIN_FILENO].data.terminal_data.ts;
    file_table[STDERR_FILENO].position = 0;
    file_table[STDERR_FILENO].flags = O_WRONLY;

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

ino_t vfs_generate_inode_number()
{
    static ino_t current_inode_number = 0;
    return current_inode_number++;
}

vfs_folder_inode_t* vfs_create_empty_folder_inode(vfs_folder_tnode_t* parent)
{
    vfs_folder_inode_t* inode = malloc(sizeof(vfs_folder_inode_t));

    inode->files = NULL;
    inode->folders = NULL;

    inode->flags = VFS_NODE_INIT;

    inode->inode_number = vfs_generate_inode_number();

    inode->parent = parent;

    return inode;
}

int vfs_root_stat(struct stat* st);
int vfs_stat(const char* path, struct stat* st);
int vfs_access(const char* path, mode_t mode);
struct dirent* vfs_readdir(struct dirent* dirent, DIR* dirp);

int vfs_read(file_entry_t* f, void* buffer, size_t num_bytes, ssize_t* bytes_read);