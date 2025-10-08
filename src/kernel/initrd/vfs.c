#pragma once

int vfs_initrd_stat(const char* path, struct stat* st)
{
    // * path | st * cant be NULL (the vfs layer already made sure)

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