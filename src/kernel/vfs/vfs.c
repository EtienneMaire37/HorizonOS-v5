#pragma once

#include "../initrd/vfs.c"
#include "vfs.h"

bool file_string_cmp(const char* s1, const char* s2)
{
    if (s1 == s2) return true;
    if (s1 == NULL || s2 == NULL) return false;
    while (*s1 && *s2 && *s1 != '/' && *s2 != '/')
    {
        if (*s1 != *s2) 
            return false;
        s1++;
        s2++;
    }
    return ((!(*s1) || (*s1 == '/')) && (!(*s2) || (*s2 == '/')));
}

size_t file_string_len(const char* str)
{
    size_t s = 0;
    while ((uint64_t)str & 3)
    {
        if ((*str == 0) || (*str == '/'))
            return s;
        str++;
        s++;
    }

    uint32_t dword;
    while (true)
    {
        dword = *(uint32_t*)str;
        if ((dword & 0xff) == 0 || (dword & 0xff) == '/')
            return s;
        if ((dword & 0xff00) == 0 || ((dword & 0xff00) >> 8) == '/')
            return s + 1;
        if ((dword & 0xff0000) == 0 || ((dword & 0xff0000) >> 16) == '/')
            return s + 2;
        if ((dword & 0xff000000) == 0 || ((dword & 0xff000000) >> 24) == '/')
            return s + 3;

        str += 4;
        s += 4;
    }
}

void vfs_explore(vfs_folder_inode_t* inode)
{
    if (!inode)
    {
        LOG(WARNING, "vfs_explore: node == NULL");
        return;
    }
    if (inode->flags & VFS_NODE_LOADING)
    {
        while (inode->flags & VFS_NODE_LOADING);
        return;
    }
    if (inode->flags & VFS_NODE_EXPLORED)
    {
        LOG(WARNING, "vfs_explore: Node already explored");
        return;
    }
    inode->flags |= VFS_NODE_LOADING;

    switch (inode->drive.type)
    {
    case DT_INITRD:
        vfs_initrd_do_explore(inode);
        break;
    default:
        abort();
    }

    inode->flags |= VFS_NODE_EXPLORED;
    inode->flags &= ~VFS_NODE_LOADING;
}

void vfs_add_chr(const char* folder, const char* name, ssize_t (*fun)(uint8_t*, size_t, uint8_t),
    uid_t uid, gid_t gid)
{
    if (!folder)
    {
        LOG(WARNING, "vfs_add_chr: folder == NULL");
        return;
    }
    if (!name)
    {
        LOG(WARNING, "vfs_add_chr: name == NULL");
        return;
    }

    vfs_folder_tnode_t* parent = vfs_get_folder_tnode(folder, NULL);
    if (!parent)
    {
        LOG(WARNING, "vfs_add_chr: \"%s\" is not a valid folder", folder);
        return;
    }

    vfs_file_tnode_t** current_tnode = &parent->inode->files;
    while (*current_tnode)
        current_tnode = &(*current_tnode)->next;

    *current_tnode = vfs_create_chr_special_file_tnode(name, parent, fun, uid, gid);
}

vfs_file_tnode_t* vfs_get_file_tnode(const char* path, vfs_folder_tnode_t* pwd)
{
    if (!path) return NULL;
    size_t path_len = strlen(path);
    if (path_len == 0) return NULL;

    if (!pwd) pwd = vfs_root;

    size_t i = 0;
    vfs_folder_inode_t* current_folder = path[0] == '/' ? (i++, vfs_root->inode) : pwd->inode;
    while (i < path_len)
    {
        if (!(current_folder->flags & VFS_NODE_EXPLORED))
            vfs_explore(current_folder);

        if (file_string_len(&path[i]) == strlen(&path[i]))
        // * Parse files
        {
            vfs_file_tnode_t* current_child = current_folder->files;
            while (current_child)
            {
                if (file_string_cmp(current_child->name, &path[i]))
                    return current_child;
                current_child = current_child->next;
            }
            return NULL;
        }

        if (file_string_cmp("", &path[i]) && path[i] != 0)
        {
            i++;
            continue;
        }
        if (file_string_cmp(".", &path[i]))
        {
            i += 2;
            continue;
        }
        if (file_string_cmp("..", &path[i]))
        {
            i += 3;
            current_folder = current_folder->parent->inode;
            continue;
        }
        vfs_folder_tnode_t* current_child = current_folder->folders;
        while (current_child)
        {
            if (file_string_cmp(current_child->name, &path[i]))
            {
                current_folder = current_child->inode;
                i += strlen(current_child->name);
                break;
            }
            current_child = current_child->next;
        }
        if (!current_child)
            return NULL;
    }
    return NULL;
}

vfs_file_inode_t* vfs_get_file_inode(const char* path, vfs_folder_tnode_t* pwd)
{
    vfs_file_tnode_t* tnode = vfs_get_file_tnode(path, pwd);
    if (!tnode) return NULL;
    return tnode->inode;
}

vfs_folder_tnode_t* vfs_get_folder_tnode(const char* path, vfs_folder_tnode_t* pwd)
{
    if (!path) return NULL;
    if (!pwd) pwd = vfs_root;

    size_t path_len = strlen(path);
    if (path_len == 0) return pwd;

    size_t i = 0;
    vfs_folder_tnode_t* current_folder = path[0] == '/' ? (i++, vfs_root) : pwd;
    while (i < path_len)
    {
        if (!(current_folder->inode->flags & VFS_NODE_EXPLORED))
            vfs_explore(current_folder->inode);

        if (file_string_cmp("", &path[i]) && path[i] != 0)
        {
            i++;
            continue;
        }
        if (file_string_cmp(".", &path[i]))
        {
            i += 2;
            continue;
        }
        if (file_string_cmp("..", &path[i]))
        {
            i += 3;
            current_folder = current_folder->inode->parent;
            continue;
        }
        vfs_folder_tnode_t* current_child = current_folder->inode->folders;
        while (current_child)
        {
            if (file_string_cmp(current_child->name, &path[i]))
            {
                current_folder = current_child;
                i += strlen(current_child->name);
                break;
            }
            current_child = current_child->next;
        }
        if (!current_child)
            return NULL;
    }
    return current_folder;
}

// int vfs_root_stat(struct stat* st)
// {
//     st->st_dev = -1;
//     st->st_ino = -1;
//     st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
//     st->st_nlink = 1;
//     st->st_uid = 0;
//     st->st_gid = 0;
//     st->st_rdev = -1;
//     st->st_size = 0;

//     st->st_blksize = 0;
//     st->st_blocks = 0;

//     st->st_atime = 0;
//     st->st_mtime = 0;
//     st->st_ctime = 0;
//     return 0;
// }

int vfs_stat(const char* path, vfs_folder_tnode_t* pwd, struct stat* st)
{
    if (!path || !st)
        return EFAULT;

    vfs_folder_tnode_t* folder_tnode = vfs_get_folder_tnode(path, pwd);

    if (folder_tnode)
    {
        *st = folder_tnode->inode->st;
        return 0;
    }

    vfs_file_tnode_t* file_tnode = vfs_get_file_tnode(path, pwd);

    if (file_tnode)
    {
        *st = file_tnode->inode->st;
        return 0;
    }

    return ENOENT;
}

int vfs_access(const char* path, vfs_folder_tnode_t* pwd, mode_t mode)
{
    if (mode == 0) return 0;
    struct stat st;
    int ret = vfs_stat(path, pwd, &st);
    if (ret)
        return ret;
    // * Assume we're the owner of every file for now
    if ((mode & R_OK) && ((st.st_mode & S_IRUSR) == 0))
        return EACCES;
    if ((mode & W_OK) && ((st.st_mode & S_IWUSR) == 0))
        return EACCES;
    if ((mode & X_OK) && ((st.st_mode & S_IXUSR) == 0))
        return EACCES;
    return 0;
}

// struct dirent* vfs_root_readdir(struct dirent* dirent, DIR* dirp)
// {
//     const char* directories[] = 
//     {
//         ".",
//         "initrd",
//         NULL
//     };
//     if (strcmp(dirp->current_entry, "") == 0)
//     {
//         strncpy(dirp->current_entry, directories[0], PATH_MAX);
//         strncpy(dirp->current_path, dirp->current_entry, PATH_MAX);
//         strncpy(dirent->d_name, dirp->current_path, PATH_MAX);
//         dirent->d_ino = -1;
//         return dirent;
//     }
//     if (strcmp(dirp->current_entry, ".") == 0)
//     {
//         strncpy(dirp->current_entry, directories[1], PATH_MAX);
//         strncpy(dirp->current_path, dirp->current_entry, PATH_MAX);
//         strncpy(dirent->d_name, dirp->current_path, PATH_MAX);
//         dirent->d_ino = -1;
//         return dirent;
//     }
//     int i = 0;
//     while (directories[i] != NULL && strcmp(dirp->current_entry, directories[i]) != 0)
//         i++;
//     if (directories[i] == NULL)
//     {
//         memset(dirp->current_path, 0, PATH_MAX);
//         memset(dirp->current_entry, 0, PATH_MAX);
//         return NULL;
//     }

//     if (directories[i + 1] == NULL)
//     {
//         memset(dirp->current_path, 0, PATH_MAX);
//         memset(dirp->current_entry, 0, PATH_MAX);
//         return NULL;
//     }

//     snprintf(dirp->current_entry, sizeof(dirp->current_entry), "%s", directories[i + 1]);
//     snprintf(dirp->current_path, sizeof(dirp->current_path), "%s", dirp->current_entry);
//     snprintf(dirent->d_name, sizeof(dirent->d_name), "%s", dirp->current_path);
//     dirent->d_ino = -1;
//     return dirent;
// }

struct dirent* vfs_readdir(struct dirent* dirent, DIR* dirp)
{
    errno = 0;
    return NULL;
}

int vfs_read(int fd, void* buffer, size_t num_bytes, ssize_t* bytes_read)
{
    if (!bytes_read) abort();
    if (__CURRENT_TASK.file_table[fd] < 0 || __CURRENT_TASK.file_table[fd] >= MAX_FILE_TABLE_ENTRIES)
    {
        *bytes_read = (uint64_t)-1;
        return EBADF;
    }
    if (!((file_table[__CURRENT_TASK.file_table[fd]].flags & O_RDONLY) || (file_table[__CURRENT_TASK.file_table[fd]].flags & O_RDWR))) 
    {
        *bytes_read = -1;
        return EBADF;
    }

    if (file_table[__CURRENT_TASK.file_table[fd]].entry_type == ET_FILE)
    {
        mode_t mode = file_table[__CURRENT_TASK.file_table[fd]].inode.file->st.st_mode;
        if (S_ISCHR(mode))
        {
            *bytes_read = file_table[__CURRENT_TASK.file_table[fd]].inode.file->file_data.chr.fun(buffer, num_bytes, CHR_DIR_READ);
            return 0;
        }
    }
    *bytes_read = 0;
    return 0;
}

int vfs_write(int fd, unsigned char* buffer, uint64_t bytes_to_write, uint64_t* bytes_written)
{
    if (!bytes_written) abort();
    if (__CURRENT_TASK.file_table[fd] < 0 || __CURRENT_TASK.file_table[fd] >= MAX_FILE_TABLE_ENTRIES)
    {
        *bytes_written = (uint64_t)-1;
        return EBADF;
    }
    if (!((file_table[__CURRENT_TASK.file_table[fd]].flags & O_WRONLY) || (file_table[__CURRENT_TASK.file_table[fd]].flags & O_RDWR))) 
    {
        *bytes_written = -1;
        return EBADF;
    }
    if (file_table[__CURRENT_TASK.file_table[fd]].entry_type == ET_FILE)
    {
        mode_t mode = file_table[__CURRENT_TASK.file_table[fd]].inode.file->st.st_mode;
        if (S_ISCHR(mode))
        {
            *bytes_written = file_table[__CURRENT_TASK.file_table[fd]].inode.file->file_data.chr.fun(buffer, bytes_to_write, CHR_DIR_WRITE);
            return 0;
        }
    }
    *bytes_written = 0;
    return 0;
}

void vfs_log_tree(vfs_folder_tnode_t* tnode, int depth)
{
    char access_str[11];

    LOG(DEBUG, "");
    for (int i = 0; i < depth; i++)
        CONTINUE_LOG(DEBUG, "    ");
    CONTINUE_LOG(DEBUG, "`%s` [inode %lld] (access: %s)%s", tnode->name, tnode->inode->st.st_ino, get_access_string(tnode->inode->st.st_mode, access_str), tnode->inode->flags & VFS_NODE_EXPLORED ? ":" : " (not explored)");
    vfs_folder_tnode_t* current_folder = tnode->inode->folders;
    while (current_folder)
    {
        vfs_log_tree(current_folder, depth + 1);
        current_folder = current_folder->next;
    }
    vfs_file_tnode_t* current_file = tnode->inode->files;
    while (current_file)
    {
        LOG(DEBUG, "");
        for (int i = 0; i < depth + 1; i++)
            CONTINUE_LOG(DEBUG, "    ");
        CONTINUE_LOG(DEBUG, "`%s` [inode %lld] (access: %s)", current_file->name, current_file->inode->st.st_ino, get_access_string(current_file->inode->st.st_mode, access_str));
        current_file = current_file->next;
    }
}