#pragma once

#include "../initrd/vfs.c"

// bool file_string_cmp(const char* s1, const char* s2)
// {
//     if (s1 == s2) return true;
//     if (s1 == NULL || s2 == NULL) return false;
//     while (*s1 && *s2 && *s1 != '/' && *s2 != '/')
//     {
//         ;
//     }
// }

// vfs_file_inode_t* vfs_get_file_inode(const char* path, vfs_folder_inode_t* pwd)
// {
//     if (!path) return NULL;
//     size_t path_len = strlen(path);
//     if (path_len == 0) return pwd;

//     size_t i = 0;
//     vfs_folder_inode_t* current_folder = path[0] == '/' ? (i++, vfs_root) : pwd;
//     while (i < path_len)
//     {
//         vfs_folder_tnode_t* current_child = current_folder->folders;
//         while (current_child)
//         {
//             bool match = true;
//             size_t current_entry_len = strlen(current_child->name);
//             for (ssize_t off = 0; off < current_entry_len; off++)
//             {
//                 if (current_child->name[off] != path[i + off])
//                 {
//                     match = false;
//                     break;
//                 }
//             }
//             if (match)
//             {
//                 current_folder = current_child->inode;
//                 break;
//             }
//             current_child = current_child->next;
//         }
//     }
// }

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

int vfs_stat(const char* path, struct stat* st)
{
    if (!path || !st)
        return EFAULT;

    // TODO: Implement this
    return ENOENT;
}

int vfs_access(const char* path, mode_t mode)
{
    if (mode == 0) return 0;
    struct stat st;
    int ret = vfs_stat(path, &st);
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

int vfs_read(file_entry_t* f, void* buffer, size_t num_bytes, ssize_t* bytes_read)
{
    // if (!bytes_read) abort();
    // if (!((f->flags & O_RDONLY) || (f->flags & O_RDWR))) 
    // {
    //     *bytes_read = -1;
    //     return EBADF;
    // }

    // switch (f->type)
    // {
    // case DT_INITRD:
    //     return vfs_initrd_read(f, buffer, num_bytes, bytes_read);
    // default:
    //     *bytes_read = -1;
    //     return EBADF;
    // }
    return EBADF;
}