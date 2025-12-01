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

static size_t vfs_realpath_from_folder_tnode_helper(vfs_folder_tnode_t* tnode, char* res, size_t idx)
{
    if (!tnode || !tnode->inode) return idx;
    if (tnode == vfs_root)
        return 0;
    
    idx = vfs_realpath_from_folder_tnode_helper(tnode->inode->parent, res, idx);
    if (idx + 1 >= PATH_MAX) return idx;
    res[idx] = '/';
    size_t len = strlen(tnode->name);
    if (len + idx + 1 >= PATH_MAX)
        len = PATH_MAX - idx - 2;
    memcpy(&res[idx + 1], tnode->name, len);
    // !! TODO: Refactor all this
    return idx + len + 1;
}

void vfs_realpath_from_folder_tnode(vfs_folder_tnode_t* tnode, char* res)
{
    if (tnode == vfs_root) 
    {
        res[0] = '/';
        res[1] = 0;
        return;
    }
    res[vfs_realpath_from_folder_tnode_helper(tnode, res, 0)] = 0;
}

void vfs_realpath_from_file_tnode(vfs_file_tnode_t* tnode, char* res)
{
    size_t ret = vfs_realpath_from_folder_tnode_helper(tnode->inode->parent, res, 0);
    res[ret] = '/';
    size_t len = strlen(tnode->name);
    memcpy(&res[ret + 1], tnode->name, len);
    res[ret + len + 1] = 0;
}

void vfs_explore(vfs_folder_tnode_t* tnode)
{
    if (!tnode || !tnode->inode)
    {
        LOG(WARNING, "vfs_explore: node == NULL");
        return;
    }
    if (tnode->inode->flags & VFS_NODE_LOADING)
    {
        while (tnode->inode->flags & VFS_NODE_LOADING);
        return;
    }
    if (tnode->inode->flags & VFS_NODE_EXPLORED)
    {
        LOG(WARNING, "vfs_explore: Node already explored");
        return;
    }
    tnode->inode->flags |= VFS_NODE_LOADING;

    switch (tnode->inode->drive.type)
    {
    case DT_INITRD:
        vfs_initrd_do_explore(tnode);
        break;
    default:
        abort();
    }

    tnode->inode->flags |= VFS_NODE_EXPLORED;
    tnode->inode->flags &= ~VFS_NODE_LOADING;
}

vfs_file_tnode_t* vfs_add_special(const char* folder, const char* name, ssize_t (*fun)(file_entry_t*, uint8_t*, size_t, uint8_t),
    uid_t uid, gid_t gid)
{
    if (!folder)
    {
        LOG(WARNING, "vfs_add_special: folder == NULL");
        return NULL;
    }
    if (!name)
    {
        LOG(WARNING, "vfs_add_special: name == NULL");
        return NULL;
    }

    vfs_folder_tnode_t* parent = vfs_get_folder_tnode(folder, NULL);
    if (!parent)
    {
        LOG(WARNING, "vfs_add_special: \"%s\" is not a valid folder", folder);
        return NULL;
    }

    vfs_file_tnode_t** current_tnode = &parent->inode->files;
    while (*current_tnode)
        current_tnode = &(*current_tnode)->next;

    *current_tnode = vfs_create_special_file_tnode(name, parent, fun, uid, gid);

    return *current_tnode;
}

vfs_file_tnode_t* vfs_get_file_tnode(const char* path, vfs_folder_tnode_t* pwd)
{
    if (!path) return NULL;
    size_t path_len = strlen(path);
    if (path_len == 0) return NULL;

    if (!pwd) pwd = vfs_root;

    size_t i = 0;
    vfs_folder_tnode_t* current_folder_tnode = path[0] == '/' ? (i++, vfs_root) : pwd;
    vfs_folder_inode_t* current_folder = current_folder_tnode->inode;
    while (i < path_len)
    {
        if (!(current_folder->flags & VFS_NODE_EXPLORED))
            vfs_explore(current_folder_tnode);

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
            current_folder_tnode = current_folder->parent;
            current_folder = current_folder_tnode->inode;
            continue;
        }
        vfs_folder_tnode_t* current_child = current_folder->folders;
        while (current_child)
        {
            if (file_string_cmp(current_child->name, &path[i]))
            {
                current_folder_tnode = current_child;
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
            vfs_explore(current_folder);

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

struct dirent* vfs_readdir(struct dirent* dirent, DIR* dirp)
{
    vfs_folder_tnode_t* folder_tnode = vfs_get_folder_tnode(dirp->path, NULL);
    if (!folder_tnode)
    {
        errno = ENOENT;
        return NULL;
    }

    if (!(folder_tnode->inode->flags & VFS_NODE_EXPLORED))
        vfs_explore(folder_tnode);

    if (strcmp(dirp->current_entry, "") == 0)
    {
        strcpy(dirp->current_entry, ".");
        memcpy(dirent->d_name, dirp->current_entry, 2);
        dirent->d_ino = folder_tnode->inode->st.st_ino;
        errno = 0;
        return dirent;
    }

    if (strcmp(dirp->current_entry, ".") == 0)
    {
        strcpy(dirp->current_entry, "..");
        memcpy(dirent->d_name, dirp->current_entry, 3);
        dirent->d_ino = folder_tnode->inode->parent->inode->st.st_ino;
        errno = 0;
        return dirent;
    }

    bool found_last_entry = strcmp(dirp->current_entry, "..") == 0;

    vfs_folder_tnode_t* current_folder = folder_tnode->inode->folders;

    while (current_folder)
    {
        if (found_last_entry)
        {
            memcpy(dirp->current_entry, current_folder->name, PATH_MAX);
            memcpy(dirent->d_name, dirp->current_entry, PATH_MAX);
            dirent->d_ino = current_folder->inode->st.st_ino;
            errno = 0;
            return dirent;
        }
        if (strcmp(dirp->current_entry, current_folder->name) == 0)
            found_last_entry = true;
        current_folder = current_folder->next;
    }

    vfs_file_tnode_t* current_file = folder_tnode->inode->files;

    while (current_file)
    {
        if (found_last_entry)
        {
            memcpy(dirp->current_entry, current_file->name, PATH_MAX);
            memcpy(dirent->d_name, dirp->current_entry, PATH_MAX);
            dirent->d_ino = current_file->inode->st.st_ino;
            errno = 0;
            return dirent;
        }
        if (strcmp(dirp->current_entry, current_file->name) == 0)
            found_last_entry = true;
        current_file = current_file->next;
    }

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
        mode_t mode = file_table[__CURRENT_TASK.file_table[fd]].tnode.file->inode->st.st_mode;
        *bytes_read = file_table[__CURRENT_TASK.file_table[fd]].tnode.file->inode->io_func(&file_table[__CURRENT_TASK.file_table[fd]], buffer, num_bytes, CHR_DIR_READ);
        if (*bytes_read > 0)
            file_table[__CURRENT_TASK.file_table[fd]].position += *bytes_read;
        return 0;
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
        mode_t mode = file_table[__CURRENT_TASK.file_table[fd]].tnode.file->inode->st.st_mode;
        *bytes_written = file_table[__CURRENT_TASK.file_table[fd]].tnode.file->inode->io_func(&file_table[__CURRENT_TASK.file_table[fd]], buffer, bytes_to_write, CHR_DIR_WRITE);
        if (*bytes_written > 0)
            file_table[__CURRENT_TASK.file_table[fd]].position += *bytes_written;
        return 0;
    }
    *bytes_written = 0;
    return 0;
}

void vfs_log_tree(vfs_folder_tnode_t* tnode, int depth)
{
    if (!tnode) return;

    char access_str[11];

    LOG(DEBUG, "");
    for (int i = 0; i < depth; i++)
        CONTINUE_LOG(DEBUG, "    ");
    CONTINUE_LOG(DEBUG, "`%s` [inode %lld] (access: %s, device id: %d)%s", tnode->name, tnode->inode->st.st_ino, get_access_string(tnode->inode->st.st_mode, access_str), tnode->inode->st.st_dev, tnode->inode->flags & VFS_NODE_EXPLORED ? ":" : " (not explored)");
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
        CONTINUE_LOG(DEBUG, "`%s` [inode %lld] (access: %s", current_file->name, current_file->inode->st.st_ino, get_access_string(current_file->inode->st.st_mode, access_str));
        if (S_ISCHR(current_file->inode->st.st_mode) || S_ISBLK(current_file->inode->st.st_mode))
            CONTINUE_LOG(DEBUG, ", special file device id: %d", tnode->inode->st.st_rdev);
        CONTINUE_LOG(DEBUG, ")");
        current_file = current_file->next;
    }
}