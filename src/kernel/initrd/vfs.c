#pragma once

int vfs_initrd_root_stat(struct stat* st)
{
    st->st_dev = -1;
    st->st_ino = -1;
    st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = -1;
    st->st_size = 0;

    st->st_blksize = 0;
    st->st_blocks = 0;

    st->st_atime = 0;
    st->st_mtime = 0;
    st->st_ctime = 0;
    return 0;
}

int vfs_initrd_stat(const char* path, struct stat* st)
{
    // * path | st * cant be NULL (the vfs layer already made sure it wasn't)

    if (strcmp(path, "") == 0)
        return vfs_initrd_root_stat(st);

    initrd_file_t* file = initrd_find_file_entry(path);
    if (!file)
        return ENOENT;
    st->st_dev = -1;
    st->st_ino = -1;
    st->st_mode = (file->type == USTAR_TYPE_DIRECTORY ? S_IFDIR : S_IFREG) | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = -1;
    st->st_size = file->size;

    st->st_blksize = 0;
    st->st_blocks = 0;

    st->st_atime = 0;
    st->st_mtime = 0;
    st->st_ctime = 0;

    return 0;
}

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

struct dirent* vfs_initrd_readdir(struct dirent* dirent, DIR* dirp)
{
    if (dirp->current_entry[0] == 0)
    {
        dirent->d_ino = -1;
        dirent->d_name[0] = '.';
        dirent->d_name[1] = 0;
        dirp->current_path[0] = '.';
        dirp->current_path[1] = 0;
        dirp->current_entry[0] = '.';
        dirp->current_entry[1] = 0;
        return dirent;
    }
    if (strcmp(dirp->current_entry, ".") == 0)
    {
        dirent->d_ino = -1;
        dirent->d_name[0] = '.';
        dirent->d_name[1] = '.';
        dirent->d_name[2] = 0;
        dirp->current_path[0] = '.';
        dirp->current_path[1] = '.';
        dirp->current_path[2] = 0;
        dirp->current_entry[0] = '.';
        dirp->current_entry[1] = '.';
        dirp->current_entry[2] = 0;
        return dirent;
    }
    bool found_last_entry = strcmp(dirp->current_entry, "..") == 0 ? true : false;
    int initrd_len = strlen("/initrd"), path_len = strlen(dirp->path);
    if (path_len < initrd_len)
        return NULL;
    int offset = path_len == initrd_len ? 0 : 1;
    for (int i = 0; i < initrd_files_count; i++)
    {
        initrd_file_t* file = &initrd_files[i];

        char* entry = file->name;
        char* n = file->name;
        while (*n)
        {
            if (*n == '/')
                entry = n + 1;
            n++;
        }

        if (vfs_initrd_file_in_directory(file->name, dirp->path + initrd_len + offset))
        {
            if (found_last_entry)
            {
                dirent->d_ino = -1;
                strncpy(dirent->d_name, entry, PATH_MAX);
                strncpy(dirp->current_path, dirent->d_name, PATH_MAX);
                strncpy(dirp->current_entry, dirent->d_name, PATH_MAX);
                return dirent;
            }

            if (strcmp(dirp->current_entry, entry) == 0)
                found_last_entry = true;
        }
    }
    return NULL;
}

int vfs_initrd_read(file_entry_t* f, void* buffer, size_t num_bytes, ssize_t* bytes_read)
{
    if (f->data.initrd_data.file == NULL)
    {
        *bytes_read = 0;
        return 0;
    }
    if (f->data.initrd_data.file->data == NULL || f->data.initrd_data.file->size == 0 || f->data.initrd_data.file->type != USTAR_TYPE_FILE_1)
    {
        *bytes_read = 0;
        return 0;
    }
    ssize_t bytes_to_read = minint(f->data.initrd_data.file->size - f->position, num_bytes);
    // LOG(DEBUG, "initrd read file entry 0x%x in to buffer 0x%x [file position %lu] reading %d bytes", f, buffer, f->position, bytes_to_read);
    if (bytes_to_read <= 0)
    {
        *bytes_read = 0;
        return 0;
    }

    memcpy(buffer, f->data.initrd_data.file->data + f->position, bytes_to_read);
    *bytes_read = bytes_to_read;
    f->position += bytes_to_read;
    return 0;
}