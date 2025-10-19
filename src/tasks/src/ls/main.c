#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdbool.h>

extern int errno;

int main(int argc, char** argv)
{
    bool l = false, a = false;

    int num_paths = 0;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            num_paths++;
            continue;
        }

        for (char* c = argv[i] + 1; *c; c++)
        {
            switch (*c)
            {
            case 'l':
                l = true;
                break;
            case 'a':
                a = true;
                break;
            default:
                fprintf(stderr, "ls: invalid argument \'%c\'\n", *c);
                return 2;
            }
        }
    }

    int paths_seen = 0;

    char* path = "./";
    if (num_paths == 0)
        goto ls_path;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
            continue;

        path = argv[i];
        if (num_paths > 1)
            printf("%s:\n", path);
        paths_seen++;
    ls_path:
        DIR* dp;
        struct dirent* ep;
        dp = opendir(path);
        if (dp != NULL)
        {
            while ((ep = readdir(dp)) != NULL)
            {
                if (!a && ep->d_name[0] == '.')
                    continue;
                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s/%s", path, ep->d_name);
                struct stat st;
                if (stat(full_path, &st) != 0)
                    continue;
                bool dir = S_ISDIR(st.st_mode);
                if (l)
                {
                    if (dir)
                        putchar('d');
                    else    
                        putchar('-');

                    if (st.st_mode & S_IRUSR)
                        putchar('r');
                    else    
                        putchar('-');
                    if (st.st_mode & S_IWUSR)
                        putchar('w');
                    else    
                        putchar('-');
                    if (st.st_mode & S_IXUSR)
                        putchar('x');
                    else    
                        putchar('-');

                    if (st.st_mode & S_IRGRP)
                        putchar('r');
                    else    
                        putchar('-');
                    if (st.st_mode & S_IWGRP)
                        putchar('w');
                    else    
                        putchar('-');
                    if (st.st_mode & S_IXGRP)
                        putchar('x');
                    else    
                        putchar('-');

                    if (st.st_mode & S_IROTH)
                        putchar('r');
                    else    
                        putchar('-');
                    if (st.st_mode & S_IWOTH)
                        putchar('w');
                    else    
                        putchar('-');
                    if (st.st_mode & S_IXOTH)
                        putchar('x');
                    else    
                        putchar('-');

                    putchar(' ');
                }
                if (dir)
                    printf("\x1b[94m");
                else
                    printf("\x1b[92m");
                printf("%s\x1b[0m\n", ep->d_name);
            }

            closedir(dp);
        }
        else
        {
            int _errno = errno;
            char buffer[256] = {0};
            snprintf(buffer, 256, "ls: cannot access \'%s\'", path);
            errno = _errno;
            perror(buffer);
        }

        if (num_paths == 0)
            return 0;

        if (paths_seen != num_paths)
            putchar('\n');
    }
}