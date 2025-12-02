#pragma once

typedef int16_t file_table_index_t;
const file_table_index_t invalid_fd = -1;

typedef enum drive_type
{
    DT_INVALID = 0,
    DT_VIRTUAL = 1,
    DT_INITRD = 2,
} drive_type_t;

const char* get_drive_type_string(drive_type_t dt)
{
    if (dt > 2 || dt < 0) dt = 0;
    return (const char*[]){"DT_INVALID", "DT_VIRTUAL", "DT_INITRD"}[dt];
}

typedef struct 
{
    drive_type_t type;
    union 
    {
        ;
    } data;
} drive_t;

// * VFS data

#define VFS_NODE_INIT       0

#define VFS_NODE_EXPLORED   1
#define VFS_NODE_LOADING    2

typedef struct vfs_file_inode vfs_file_inode_t;

typedef struct vfs_file_tnode vfs_file_tnode_t;
typedef struct vfs_folder_tnode vfs_folder_tnode_t;

typedef struct file_entry file_entry_t;

typedef struct 
{
    uint8_t ide_idx, ata_idx;
} ide_descriptor_t;

// * i-nodes
typedef struct vfs_file_inode
{
    drive_t drive;

    union
    {
        initrd_file_t* initrd;
        ide_descriptor_t ide;
    } file_data;

    ssize_t (*io_func)(file_entry_t*, uint8_t* buf, size_t count, uint8_t direction);

    struct stat st;

    vfs_folder_tnode_t* parent;
} vfs_file_inode_t;

typedef struct
{
    drive_t drive;

    struct stat st;

    vfs_file_tnode_t* files;
    vfs_folder_tnode_t* folders;

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

vfs_folder_tnode_t* vfs_root = NULL;

// * Open file descriptor data

#define ET_FILE     1
#define ET_FOLDER   2

typedef struct file_entry
{
    int used;
    int flags;
    off_t position;
    uint8_t entry_type;
    union
    {
        vfs_file_tnode_t* file;
        vfs_folder_tnode_t* folder;
    } tnode;
} file_entry_t;

#define MAX_FILE_TABLE_ENTRIES  256

file_entry_t file_table[MAX_FILE_TABLE_ENTRIES];

atomic_flag file_table_spinlock = ATOMIC_FLAG_INIT;

#define CHR_DIR_READ    1
#define CHR_DIR_WRITE   2

vfs_file_inode_t* vfs_get_file_inode(const char* path, vfs_folder_tnode_t* pwd);
vfs_file_tnode_t* vfs_get_file_tnode(const char* path, vfs_folder_tnode_t* pwd);
vfs_folder_tnode_t* vfs_get_folder_tnode(const char* path, vfs_folder_tnode_t* pwd);

void vfs_init_file_table()
{
    acquire_spinlock(&file_table_spinlock);
    for (int i = 0; i < MAX_FILE_TABLE_ENTRIES; i++)
    {
        if (i < 3)  file_table[i].used = 1; // * Should always be used
        else        file_table[i].used = 0;
    }
    
    file_table[0].entry_type = ET_FILE;
    file_table[0].tnode.file = vfs_get_file_tnode("/devices/stdin", NULL);
    file_table[0].position = 0;
    file_table[0].flags = O_RDONLY;

    file_table[1].entry_type = ET_FILE;
    file_table[1].tnode.file = vfs_get_file_tnode("/devices/stdout", NULL);
    file_table[1].position = 0;
    file_table[1].flags = O_WRONLY;

    file_table[2].entry_type = ET_FILE;
    file_table[2].tnode.file = vfs_get_file_tnode("/devices/stderr", NULL);
    file_table[2].position = 0;
    file_table[2].flags = O_WRONLY;

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
            file_table[fd].used = 0;
        release_spinlock(&file_table_spinlock);
        return;
    }
    release_spinlock(&file_table_spinlock);
}

ino_t vfs_generate_inode_number()
{
    static ino_t current_inode_number = 1;
    return current_inode_number++;
}

dev_t vfs_generate_device_id()
{
    static dev_t current_device_id = 1;
    return current_device_id++;
}

vfs_folder_inode_t* vfs_create_empty_folder_inode(vfs_folder_tnode_t* parent, uint8_t flags,
    dev_t device_id, mode_t mode, uid_t uid, gid_t gid,
    drive_t drive)
{
    vfs_folder_inode_t* inode = malloc(sizeof(vfs_folder_inode_t));

    inode->files = NULL;
    inode->folders = NULL;

    inode->flags = flags;

    inode->st.st_dev = device_id;
    inode->st.st_ino = vfs_generate_inode_number();
    inode->st.st_mode = mode;
    inode->st.st_nlink = 1;
    inode->st.st_uid = uid;
    inode->st.st_gid = gid;
    inode->st.st_rdev = 0;
    inode->st.st_size = 0;
    inode->st.st_blksize = 0;
    inode->st.st_blocks = 0;
    inode->st.st_atime = 0;
    inode->st.st_mtime = 0;
    inode->st.st_ctime = 0;

    inode->drive = drive;

    inode->parent = parent;

    return inode;
}

vfs_folder_tnode_t* vfs_create_empty_folder_tnode(const char* name, vfs_folder_tnode_t* parent, uint8_t flags,
    dev_t device_id, mode_t mode, uid_t uid, gid_t gid,
    drive_t drive)
{
    vfs_folder_tnode_t* tnode = malloc(sizeof(vfs_folder_tnode_t));
    if (!tnode)
    {
        LOG(ERROR, "Couldn't allocate tnode");
        return NULL;
    }
    tnode->name = strdup(name);
    if (!tnode->name)
    {
        free(tnode);
        LOG(ERROR, "Couldn't allocate name");
        return NULL;
    }
    tnode->next = NULL;
    tnode->inode = vfs_create_empty_folder_inode(parent, flags, device_id, mode, uid, gid, drive);
    if (!tnode->inode)
    {
        free(tnode->name);
        free(tnode);
        LOG(ERROR, "Couldn't allocate inode");
        return NULL;
    }
    return tnode;
}

vfs_file_inode_t* vfs_create_special_file_inode(vfs_folder_tnode_t* parent, ssize_t (*fun)(file_entry_t*, uint8_t*, size_t, uint8_t), uid_t uid, gid_t gid)
{
    vfs_file_inode_t* inode = malloc(sizeof(vfs_file_inode_t));
    if (!inode) return NULL;

    inode->drive.type = DT_VIRTUAL;
    inode->io_func = fun;
    inode->parent = parent;
    
    inode->st.st_dev = 0;   // * root device
    inode->st.st_ino = vfs_generate_inode_number();
    inode->st.st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    inode->st.st_nlink = 1;
    inode->st.st_uid = uid;
    inode->st.st_gid = gid;
    inode->st.st_rdev = vfs_generate_device_id();
    inode->st.st_size = 0;
    inode->st.st_blksize = 0;
    inode->st.st_blocks = 0;
    inode->st.st_atime = 0;
    inode->st.st_mtime = 0;
    inode->st.st_ctime = 0;

    return inode;
}

vfs_file_tnode_t* vfs_create_special_file_tnode(const char* name, vfs_folder_tnode_t* parent, ssize_t (*fun)(file_entry_t*, uint8_t*, size_t, uint8_t), uid_t uid, gid_t gid)
{
    vfs_file_tnode_t* tnode = malloc(sizeof(vfs_file_tnode_t));
    if (!tnode)
    {
        LOG(ERROR, "Couldn't allocate tnode");
        return NULL;
    }
    tnode->name = strdup(name);
    if (!tnode->name)
    {
        free(tnode);
        LOG(ERROR, "Couldn't allocate name");
        return NULL;
    }
    tnode->next = NULL;
    tnode->inode = vfs_create_special_file_inode(parent, fun, uid, gid);
    if (!tnode->inode)
    {
        free(tnode->name);
        free(tnode);
        LOG(ERROR, "Couldn't allocate inode");
        return NULL;
    }
    return tnode;
}

void vfs_mount_device(const char* name, drive_t drive, uid_t uid, gid_t gid)
{
    LOG(DEBUG, "Mounting device %s at /%s", get_drive_type_string(drive.type), name);

    vfs_folder_tnode_t** current = &vfs_root->inode->folders;

    if (!(*current)) 
        goto mount;

    while (*current)
    {
        if (strcmp(name, (*current)->name) == 0)
        {
            LOG(ERROR, "vfs_mount_device: Couldn't mount partition: Mount point already exists");
            return;
        }

        current = &(*current)->next;
    }

mount: 
    (*current) = vfs_create_empty_folder_tnode(name, vfs_root, 
            VFS_NODE_INIT, 
            drive.type == DT_VIRTUAL ? 0 : vfs_generate_device_id(), 
            S_IFDIR | 
            S_IRUSR | S_IXUSR |
            S_IRGRP | S_IXGRP |
            S_IROTH | S_IXOTH, 
            uid, gid,
            drive);
    
    return;
}

static void vfs_unload_folder_helper(vfs_folder_tnode_t* tnode)
{
    if (!tnode) return;
    if (tnode->inode->parent == tnode) return;

    while (tnode->inode->folders)
    {
        vfs_folder_tnode_t* next_folder_tnode = tnode->inode->folders->next;
        vfs_unload_folder_helper(tnode->inode->folders);
        tnode->inode->folders = next_folder_tnode;
    }

    while (tnode->inode->files)
    {
        vfs_file_tnode_t* file_tnode = tnode->inode->files; 
        free(file_tnode->inode);
        free(file_tnode->name);
        tnode->inode->files = tnode->inode->files->next;
        free(file_tnode);
    }

    free(tnode->inode);
    free(tnode->name);
    free(tnode);
}

void vfs_unload_folder(vfs_folder_tnode_t* tnode)
{
    vfs_folder_tnode_t* parent = tnode->inode->parent;
    vfs_folder_tnode_t** current_folder = &parent->inode->folders;
    while (*current_folder && (*current_folder) != tnode)
        current_folder = &(*current_folder)->next;
    if (!*current_folder)
        abort();     // !!! Should be impossible
    *current_folder = tnode->next;
    vfs_unload_folder_helper(tnode);
}

ssize_t task_chr_stdin(file_entry_t* entry, uint8_t* buf, size_t count, uint8_t direction);
ssize_t task_chr_stdout(file_entry_t* entry, uint8_t* buf, size_t count, uint8_t direction);
ssize_t task_chr_stderr(file_entry_t* entry, uint8_t* buf, size_t count, uint8_t direction);

ssize_t initrd_iofunc(file_entry_t* entry, uint8_t* buf, size_t count, uint8_t direction);

bool vfs_isatty(file_entry_t* entry)
{
    return entry->entry_type == ET_FILE ? (S_ISCHR(entry->tnode.file->inode->st.st_mode) && (entry->tnode.file->inode->io_func == task_chr_stdin || entry->tnode.file->inode->io_func == task_chr_stdout || entry->tnode.file->inode->io_func == task_chr_stderr)) : false;
}

vfs_file_tnode_t* vfs_add_special(const char* folder, const char* name, ssize_t (*fun)(file_entry_t*, uint8_t*, size_t, uint8_t),
    uid_t uid, gid_t gid);

void vfs_realpath_from_folder_tnode(vfs_folder_tnode_t* inode, char* res);
void vfs_realpath_from_file_tnode(vfs_file_tnode_t* tnode, char* res);

bool file_string_cmp(const char* s1, const char* s2);

int vfs_root_stat(struct stat* st);
int vfs_stat(const char* path, vfs_folder_tnode_t* pwd, struct stat* st);
int vfs_access(const char* path, vfs_folder_tnode_t* pwd, mode_t mode);
struct dirent* vfs_readdir(struct dirent* dirent, DIR* dirp);

int vfs_read(int fd, void* buffer, size_t num_bytes, ssize_t* bytes_read);
int vfs_write(int fd, unsigned char* buffer, uint64_t bytes_to_write, uint64_t* bytes_written);

void vfs_log_tree();