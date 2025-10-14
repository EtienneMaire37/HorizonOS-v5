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

    const char* initrd_prefix = "/initrd";

    if (strcmp(initrd_prefix, path) == 0)
        return vfs_initrd_root_stat(st);

    size_t i = 0;
    while (path[i] != 0 && initrd_prefix[i] != 0 && path[i] == initrd_prefix[i])
        i++;
    const size_t len = strlen(initrd_prefix);
    // printf("%s | %s\n", path, initrd_prefix);
    if (i == len)
        return vfs_initrd_stat((char*)((uint32_t)path + len + 1), st);

    return ENOENT;
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

    // LOG(DEBUG, "\"%s\" - \"%s\" : \"%s\"", dirp->path, dirp->current_path, dirp->current_entry);

    if (strcmp(dirp->path, "/") == 0)
        return vfs_root_readdir(dirent, dirp);

    const char* initrd_prefix = "/initrd";

    size_t i = 0;
    while (dirp->path[i] != 0 && initrd_prefix[i] != 0 && dirp->path[i] == initrd_prefix[i])
        i++;
    const size_t len = strlen(initrd_prefix);
    if (i == len)
        return vfs_initrd_readdir(dirent, dirp);
        
    return NULL;
}