#pragma once

int vfs_root_stat(struct stat* st);
int vfs_stat(const char* path, struct stat* st);
int vfs_access(const char* path, mode_t mode);
struct dirent* vfs_readdir(struct dirent* dirent, DIR* dirp);