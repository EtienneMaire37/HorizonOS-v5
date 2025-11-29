#pragma once

#include "../vfs/vfs.h"

bool vfs_initrd_file_in_directory(char* fname, const char* direc) 
{
    if (!fname || !direc) return false;

    // LOG(DEBUG, "\"%s\" | \"%s\"", fname, direc);

    if (*direc == 0) 
    {
        for (char* f = fname; *f; f++) 
            if (*f == '/')
                return false;
        return true;
    }

    const char* f = fname;
    const char* d = direc;

    while (*d && *f && (*f == *d)) 
    {
        f++;
        d++;
    }

    if (*d) 
        return false;

    if (*f != '/') 
        return false;

    f++;

    while (*f) 
    {
        if (*f == '/') return false;
        f++;
    }

    return true;
}

void vfs_initrd_do_explore(vfs_folder_tnode_t* tnode)
{
    if (tnode->inode->drive.type != DT_INITRD) 
    {
        LOG(ERROR, "vfs_initrd_do_explore: not an initrd mounted folder!!!");
        return;
    }
    char contructed_path[PATH_MAX];
    vfs_realpath_from_folder_tnode(tnode, contructed_path);
    // LOG(DEBUG, "Exploring \"%s\"", contructed_path);
    tnode->inode->files = NULL;
    tnode->inode->folders = NULL;
    char* path = strcmp(contructed_path, "/initrd") == 0 ? "" : &contructed_path[strlen("/initrd/")];
    vfs_file_tnode_t** current_file_tnode = &tnode->inode->files;
    vfs_folder_tnode_t** current_folder_tnode = &tnode->inode->folders;
    for (int i = 0; i < initrd_files_count; i++)
    {
        if (vfs_initrd_file_in_directory(initrd_files[i].name, path))
        {
            if (initrd_files[i].type != USTAR_TYPE_FILE_1 && initrd_files[i].type != USTAR_TYPE_DIRECTORY) continue;
            char* name = initrd_files[i].name;
            for (ssize_t j = 0; name[j] != 0; j++)
            {
                if (name[j] == '/' && name[j + 1] != 0)
                {
                    name = &name[j + 1];
                    j = -1;
                    continue;
                }
            }
            // LOG(DEBUG, "%s", name);
            switch (initrd_files[i].type)
            {
            case USTAR_TYPE_FILE_1:
                *current_file_tnode = malloc(sizeof(vfs_file_tnode_t));

                (*current_file_tnode)->name = malloc(strlen(name) + 1);
                memcpy((*current_file_tnode)->name, name, strlen(name) + 1);

                (*current_file_tnode)->inode = malloc(sizeof(vfs_file_inode_t));
                (*current_file_tnode)->inode->drive.type = DT_INITRD;
                (*current_file_tnode)->inode->st = initrd_files[i].st;
                (*current_file_tnode)->inode->st.st_ino = vfs_generate_inode_number();
                (*current_file_tnode)->inode->st.st_dev = tnode->inode->st.st_dev;
                (*current_file_tnode)->inode->st.st_rdev = 0; // S_ISCHR(initrd_files[i].st.st_mode) || S_ISBLK(initrd_files[i].st.st_mode) ? vfs_generate_device_id() : 0;
                // !!! TODO: Make it so each file inode has a pointer to a func for doing io (not just special files)
                // !!! Also refactor this to use an empty inode helper function
                
                (*current_file_tnode)->next = NULL;
                (*current_file_tnode)->inode->parent = tnode;
                current_file_tnode = &(*current_file_tnode)->next;
                break;

            case USTAR_TYPE_DIRECTORY:
                *current_folder_tnode = malloc(sizeof(vfs_folder_tnode_t));

                (*current_folder_tnode)->name = malloc(strlen(name) + 1);
                memcpy((*current_folder_tnode)->name, name, strlen(name) + 1);

                (*current_folder_tnode)->inode = malloc(sizeof(vfs_folder_inode_t));
                (*current_folder_tnode)->inode->drive.type = DT_INITRD;
                (*current_folder_tnode)->inode->st = initrd_files[i].st;
                (*current_folder_tnode)->inode->st.st_ino = vfs_generate_inode_number();
                (*current_folder_tnode)->inode->st.st_dev = tnode->inode->st.st_dev;
                (*current_folder_tnode)->inode->st.st_rdev = S_ISCHR((*current_folder_tnode)->inode->st.st_mode) || S_ISBLK((*current_folder_tnode)->inode->st.st_mode) ? vfs_generate_device_id() : 0;

                (*current_folder_tnode)->inode->flags = VFS_NODE_INIT;

                (*current_folder_tnode)->next = NULL;
                (*current_folder_tnode)->inode->parent = tnode;
                current_folder_tnode = &(*current_folder_tnode)->next;
                break;

            default:
            }
        }
    }
}

// int vfs_initrd_root_stat(struct stat* st)
// {
//     st->st_dev = -1;
//     st->st_ino = -1;
//     st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;  // * dr-xr-xr-x
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

// int vfs_initrd_stat(const char* path, struct stat* st)
// {
//     // * path | st * cant be NULL (the vfs layer already made sure it wasn't)

//     if (strcmp(path, "") == 0)
//         return vfs_initrd_root_stat(st);

//     initrd_file_t* file = initrd_find_file_entry(path);
//     if (!file)
//         return ENOENT;

//     st->st_dev = -1;
//     st->st_ino = -1;
//     st->st_mode = file->mode;
//     st->st_nlink = 1;
//     st->st_uid = 0;
//     st->st_gid = 0;
//     st->st_rdev = -1;
//     st->st_size = file->size;

//     st->st_blksize = 0;
//     st->st_blocks = 0;

//     st->st_atime = 0;
//     st->st_mtime = 0;
//     st->st_ctime = 0;

//     return 0;
// }

// struct dirent* vfs_initrd_readdir(struct dirent* dirent, DIR* dirp)
// {
//     if (dirp->current_entry[0] == 0)
//     {
//         dirent->d_ino = -1;
//         dirent->d_name[0] = '.';
//         dirent->d_name[1] = 0;
//         dirp->current_path[0] = '.';
//         dirp->current_path[1] = 0;
//         dirp->current_entry[0] = '.';
//         dirp->current_entry[1] = 0;
//         return dirent;
//     }
//     if (strcmp(dirp->current_entry, ".") == 0)
//     {
//         dirent->d_ino = -1;
//         dirent->d_name[0] = '.';
//         dirent->d_name[1] = '.';
//         dirent->d_name[2] = 0;
//         dirp->current_path[0] = '.';
//         dirp->current_path[1] = '.';
//         dirp->current_path[2] = 0;
//         dirp->current_entry[0] = '.';
//         dirp->current_entry[1] = '.';
//         dirp->current_entry[2] = 0;
//         return dirent;
//     }
//     bool found_last_entry = strcmp(dirp->current_entry, "..") == 0 ? true : false;
//     int initrd_len = strlen("/initrd"), path_len = strlen(dirp->path);
//     // LOG(DEBUG, "PATH: %s", dirp->path);
//     // LOG(DEBUG, "CURRENT: %s", dirp->current_entry);
//     // LOG(DEBUG, "CURRENTP: %s", dirp->current_path);
//     if (path_len < initrd_len)
//         return NULL;
//     int offset = path_len == initrd_len ? 0 : 1;
//     for (int i = 0; i < initrd_files_count; i++)
//     {
//         initrd_file_t* file = &initrd_files[i];

//         char* entry = file->name;
//         char* n = file->name;
//         while (*n)
//         {
//             if (*n == '/')
//                 entry = n + 1;
//             n++;
//         }

//         if (vfs_initrd_file_in_directory(file->name, dirp->path + initrd_len + offset))
//         {
//             if (found_last_entry)
//             {
//                 dirent->d_ino = -1;
//                 snprintf(dirent->d_name, sizeof(dirent->d_name), "%s", entry);
//                 snprintf(dirp->current_path, sizeof(dirp->current_path), "%s", dirent->d_name);
//                 snprintf(dirp->current_entry, sizeof(dirp->current_entry), "%s", dirent->d_name);
//                 return dirent;
//             }

//             if (strcmp(dirp->current_entry, entry) == 0)
//                 found_last_entry = true;
//         }
//     }
//     return NULL;
// }

// int vfs_initrd_read(file_entry_t* f, void* buffer, size_t num_bytes, ssize_t* bytes_read)
// {
//     if (f->data.initrd_data.file == NULL)
//     {
//         *bytes_read = 0;
//         return 0;
//     }
//     if (f->data.initrd_data.file->data == NULL || f->data.initrd_data.file->size == 0 || f->data.initrd_data.file->type != USTAR_TYPE_FILE_1)
//     {
//         *bytes_read = 0;
//         return 0;
//     }
//     ssize_t bytes_to_read = minint(f->data.initrd_data.file->size - f->position, num_bytes);
   
//     if (bytes_to_read <= 0)
//     {
//         *bytes_read = 0;
//         return 0;
//     }

//     memcpy(buffer, f->data.initrd_data.file->data + f->position, bytes_to_read);
//     *bytes_read = bytes_to_read;
//     f->position += bytes_to_read;
//     return 0;
// }