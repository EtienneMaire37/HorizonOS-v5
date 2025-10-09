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

    int i = 0;
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