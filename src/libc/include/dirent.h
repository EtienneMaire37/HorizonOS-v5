#pragma once

struct dirent
{
    ino_t d_ino;                // File serial number.
    char d_name[PATH_MAX];      // Filename string of entry.
};

typedef struct
{
    char path[PATH_MAX];
    char current_path[PATH_MAX];
} DIR;

DIR* opendir(const char* name);
int closedir(DIR* dirp);
struct dirent* readdir(DIR* dirp);