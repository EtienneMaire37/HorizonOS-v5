#pragma once

int vfs_root_stat(struct stat* st)
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

int vfs_stat(const char* path, struct stat* st)
{
    if (!path || !st)
        return EFAULT;

    if (strcmp(path, "/") == 0)
        return vfs_root_stat(st);

    switch (get_drive_type(path))
    {
    case DT_INITRD:
        return vfs_initrd_stat((char*)((uint32_t)path + strlen("/initrd") + 1), st);
    default:
        return ENOENT;
    }
}

int vfs_access(const char* path, mode_t mode)
{
    if (mode == 0) return 0;
    struct stat st;
    int ret = vfs_stat(path, &st);
    if (ret)
        return ret;
    if ((mode & R_OK) && ((st.st_mode & S_IRUSR) == 0))
        return EACCES;
    if ((mode & W_OK) && ((st.st_mode & S_IWUSR) == 0))
        return EACCES;
    if ((mode & X_OK) && ((st.st_mode & S_IXUSR) == 0))
        return EACCES;
    return 0;
}

struct dirent* vfs_root_readdir(struct dirent* dirent, DIR* dirp)
{
    const char* directories[] = 
    {
        ".",
        "initrd",
        NULL
    };
    if (strcmp(dirp->current_entry, "") == 0)
    {
        strncpy(dirp->current_entry, directories[0], PATH_MAX);
        strncpy(dirp->current_path, dirp->current_entry, PATH_MAX);
        strncpy(dirent->d_name, dirp->current_path, PATH_MAX);
        dirent->d_ino = -1;
        return dirent;
    }
    if (strcmp(dirp->current_entry, ".") == 0)
    {
        strncpy(dirp->current_entry, directories[1], PATH_MAX);
        strncpy(dirp->current_path, dirp->current_entry, PATH_MAX);
        strncpy(dirent->d_name, dirp->current_path, PATH_MAX);
        dirent->d_ino = -1;
        return dirent;
    }
    int i = 0;
    while (directories[i] != NULL && strcmp(dirp->current_entry, directories[i]) != 0)
        i++;
    if (directories[i] == NULL)
    {
        memset(dirp->current_path, 0, PATH_MAX);
        memset(dirp->current_entry, 0, PATH_MAX);
        return NULL;
    }

    if (directories[i + 1] == NULL)
    {
        memset(dirp->current_path, 0, PATH_MAX);
        memset(dirp->current_entry, 0, PATH_MAX);
        return NULL;
    }

    strncpy(dirp->current_entry, directories[i + 1], PATH_MAX);
    strncpy(dirp->current_path, dirp->current_entry, PATH_MAX);
    strncpy(dirent->d_name, dirp->current_path, PATH_MAX);
    dirent->d_ino = -1;
    return dirent;
}

struct dirent* vfs_readdir(struct dirent* dirent, DIR* dirp)
{
    errno = 0;
    if (!dirent)
        return NULL;
    if (!dirp)
    {
        errno = EBADF;
        return NULL;
    }

    if (strcmp(dirp->path, "/") == 0)
        return vfs_root_readdir(dirent, dirp);

    switch (get_drive_type(dirp->path))
    {
    case DT_INITRD:
        return vfs_initrd_readdir(dirent, dirp);
    default:
        errno = ENOENT;
        return NULL;
    }
}

int vfs_read(file_entry_t* f, void* buffer, size_t num_bytes, ssize_t* bytes_read)
{
    if (!bytes_read) abort();
    if (!((f->flags & O_RDONLY) || (f->flags & O_RDWR))) 
    {
        *bytes_read = -1;
        return EBADF;
    }

    switch (f->type)
    {
    case DT_INITRD:
        return vfs_initrd_read(f, buffer, num_bytes, bytes_read);
    default:
        *bytes_read = -1;
        return EBADF;
    }
}