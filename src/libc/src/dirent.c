#pragma once

DIR* opendir(const char* name)
{
    char* rp = realpath(name, NULL);
    if (!rp)
    {
        errno = ENOMEM;
        return NULL;
    }
    struct stat st;
    if (stat(rp, &st) != 0)
        return NULL;

    if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        return NULL;
    }

    DIR* d = malloc(sizeof(DIR));
    strncpy(d->path, rp, PATH_MAX);
    memset(d->current_path, 0, PATH_MAX);
    return d;
}

int closedir(DIR* dirp)
{
    free(dirp);
    return 0;
}

struct dirent* readdir(DIR* dirp)
{
    return NULL;
}