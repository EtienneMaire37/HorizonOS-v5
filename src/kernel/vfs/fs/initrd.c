#include "initrd.h"
#include "../inode.h"
#include "../util/string.h"
#include "../util/assert.h"
#include "../log.h"
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

typedef char tar_file_type;

typedef struct initrd_file
{
    char* name;
    uint8_t* data;
    struct stat st;
    char* link;
} initrd_file_t;

typedef struct __attribute__((packed))
{
    char        name[100];
    uint8_t     mode[8];
    uint64_t    owner_id;
    uint64_t    group_id;
    char        size[12];
    uint8_t     last_modification[12];
    uint8_t     checksum[8];
    uint8_t     type;
    char        linked_file[100];
    uint8_t     ustar[6];
    uint8_t     version[2];
    uint8_t     owner_name[32];
    uint8_t     group_name[32];
    uint8_t     device_major[8];
    uint8_t     device_minor[8];
    uint8_t     filename_prefix[155];
    uint8_t     padding[12];
} ustar_header_t;

#define MAX_INITRD_FILES 256

initrd_file_t initrd_files[MAX_INITRD_FILES];
size_t initrd_file_count = 0;

#define USTAR_TYPE_FILE_1           '0'
#define USTAR_TYPE_FILE_2            0
#define USTAR_TYPE_HARD_LINK        '1'
#define USTAR_TYPE_SYMBOLIC_LINK    '2'
#define USTAR_TYPE_CHARACTER_DEVICE '3'
#define USTAR_TYPE_BLOCK_DEVICE     '4'
#define USTAR_TYPE_DIRECTORY        '5'
#define USTAR_TYPE_NAMED_PIPE       '6'

#define USTAR_IS_VALID_HEADER(header) ((header).ustar[0] == 'u' && (header).ustar[1] == 's' && (header).ustar[2] == 't' && (header).ustar[3] == 'a' && (header).ustar[4] == 'r')

#define TSUID 	04000 	// set user ID on execution
#define TSGID 	02000 	// set group ID on execution
#define TSVTX 	01000 	// reserved
#define TUREAD 	00400 	// read by owner
#define TUWRITE 00200 	// write by owner
#define TUEXEC 	00100 	// execute or search by owner
#define TGREAD 	00040 	// read by group
#define TGWRITE 00020 	// write by group
#define TGEXEC 	00010 	// execute or search by group
#define TOREAD 	00004 	// read by others
#define TOWRITE 00002 	// write by others
#define TOEXEC 	00001 	// execute or search by other

static inline uint64_t ustar_get_number(char* str, int characters)
{
    uint64_t result = 0;
    uint64_t count = 1;

    for (uint8_t j = characters - 1; j > 0; j--, count *= 8)
        result += ((str[j - 1] - '0') * count);

    return result;
}

ino_t initrd_generate_ino()
{
    static ino_t num = 1;
    return num++;
}

int initrd_explore(vnode_ref_t vnode)
{
    char path[PATH_MAX];
    vfs_get_relative_path_to_node_from_mountpoint(vnode, path, sizeof(path));
    for (size_t i = 0; i < initrd_file_count; i++)
    {
        if (str_starts_with(initrd_files[i].name, path))
        {
            const char* node_name = &initrd_files[i].name[strlen(path) + 1];
            size_t* fs_specific = malloc(sizeof(size_t));
            *fs_specific = i;
            vfs_add_new_child_node(vnode, node_name, &initrd_files[i].st, fs_specific, free);
        }
    }
    return 0;
}
ssize_t initrd_read(inode_ref_t inode, void* buf, size_t count, off_t offset)
{
    ASSERT(inode.ptr);
    if (offset < 0) return EINVAL;
    initrd_file_t* file = &initrd_files[*(size_t*)inode.ptr->fs_specific];
    size_t filesize =    S_ISLNK(file->st.st_mode) ? strlen(file->link) :
                        (S_ISREG(file->st.st_mode) ? (size_t)file->st.st_size : 0);
    if ((size_t)offset >= filesize)
        return 0;
    if ((size_t)(offset + count) > filesize)
        count = filesize - offset;
    memcpy(buf, (S_ISREG(file->st.st_mode) ? file->data : (uint8_t*)file->link) + offset, count);
    return count;
}
ssize_t initrd_write(inode_ref_t inode, void* buf, size_t count, off_t offset)
{
    (void)inode;
    (void)buf;
    (void)count;
    (void)offset;
    return -EROFS;
}

void initrd_init(const char* path)
{
    int fd = open(path, O_RDONLY);
    ASSERT(fd != -1);
    ASSERT(sizeof(ustar_header_t) == 512);
    ustar_header_t header;
    while (read(fd, &header, 512) == 512)
    {
        if (!USTAR_IS_VALID_HEADER(header))
            break;
        size_t filesize = ustar_get_number(header.size, sizeof(header.size));
        size_t blocks = (filesize + 511) / 512;
        // Skip "./"
        if (!(header.name[0] != 0 && header.name[1] != 0))
        {
            lseek(fd, blocks * 512, SEEK_CUR);
            continue;
        }
        initrd_files[initrd_file_count].name = strdup(header.name + 1);
        size_t len = strlen(initrd_files[initrd_file_count].name);
        if (initrd_files[initrd_file_count].name[len - 1] == '/')
            initrd_files[initrd_file_count].name[len - 1] = 0;
        initrd_files[initrd_file_count].data = malloc(blocks * 512);
        if (read(fd, initrd_files[initrd_file_count].data, blocks * 512) == -1)
        {
            LOG(ERROR, "Coudln't read file data from initrd");
            abort();
        }
        initrd_files[initrd_file_count].link = strdup(header.linked_file);
        initrd_files[initrd_file_count].st.st_nlink = 1;
        initrd_files[initrd_file_count].st.st_blksize = 512;
        initrd_files[initrd_file_count].st.st_blocks = blocks;
        initrd_files[initrd_file_count].st.st_uid = header.owner_id;
        initrd_files[initrd_file_count].st.st_gid = header.group_id;
        initrd_files[initrd_file_count].st.st_ino = initrd_generate_ino();
        initrd_files[initrd_file_count].st.st_size = filesize;
        mode_t filetype;
        switch (header.type)
        {
        case USTAR_TYPE_FILE_1:
        case USTAR_TYPE_FILE_2:
            filetype = S_IFREG;
            break;
        case USTAR_TYPE_DIRECTORY:
            filetype = S_IFDIR;
            break;
        case USTAR_TYPE_SYMBOLIC_LINK:
            filetype = S_IFLNK;
            break;
        default:
            LOG(ERROR, "Unsupported ustar file type in initrd (%d)", header.type);
            abort();
        }
        uint64_t mode = ustar_get_number((char*)header.mode, 8);
        initrd_files[initrd_file_count].st.st_mode = filetype |
            ((mode & TUREAD) ? S_IRUSR : 0) | ((mode & TUEXEC) ? S_IXUSR : 0) | // * | ((mode & TUWRITE) ? S_IWUSR : 0)
            ((mode & TGREAD) ? S_IRGRP : 0) | ((mode & TGEXEC) ? S_IXGRP : 0) | // * | ((mode & TGWRITE) ? S_IWGRP : 0)
            ((mode & TOREAD) ? S_IROTH : 0) | ((mode & TOEXEC) ? S_IXOTH : 0) | // * | ((mode & TOWRITE) ? S_IWOTH : 0)
            ((mode & TSUID)  ? S_ISUID : 0) | ((mode & TSGID)  ? S_ISGID : 0)
                                            | ((mode & TSVTX)  ? S_ISVTX : 0);
        
        initrd_files[initrd_file_count].st.st_atim = (struct timespec){ 0, 0 };
        initrd_files[initrd_file_count].st.st_ctim = (struct timespec){ 0, 0 };
        initrd_files[initrd_file_count].st.st_mtim = (struct timespec){ 0, 0 };
        initrd_file_count++;
    }
    close(fd);
}

void __attribute__((destructor)) initrd_free()
{
    for (size_t i = 0; i < initrd_file_count; i++)
    {
        free(initrd_files[i].data);
        free(initrd_files[i].link);
        free(initrd_files[i].name);
    }
}

int initrd_create(const char* name, vnode_ref_t parent, struct stat* st, void** data, void (**free_data)(void*))
{
    (void)name;
    (void)parent;
    (void)st;
    (void)data;
    (void)free_data;
    return EROFS;
}

inode_ref_t initrd_create_root_inode(mountpoint_t* mp)
{
    mountpoint_ref_t mp_ref = (mountpoint_ref_t){ mp };
    struct stat st;
    st.st_atim = (struct timespec){ 0, 0 };
    st.st_ctim = (struct timespec){ 0, 0 };
    st.st_mtim = (struct timespec){ 0, 0 };
    st.st_blksize = 4096;
    st.st_blocks = 0;
    st.st_rdev = 0;

    st.st_gid = 0;
    st.st_uid = 0;

    st.st_ino = initrd_generate_ino();
    st.st_mode = 0755 | S_IFDIR;

    st.st_size = 0;

    inode_ref_t inode = vfs_create_new_inode(&st, NULL, NULL, mp_ref);
    if (!inode.ptr) return inode;
    inode.ptr->st.st_nlink = 1;
    return inode;
}
