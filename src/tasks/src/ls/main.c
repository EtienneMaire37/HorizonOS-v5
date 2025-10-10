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

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            fprintf(stderr, "ls: invalid argument \"%s\"\n", argv[i]);
            exit(1);
        }

        for (char* c = argv[i] + 1; *c; c++)
        {
            switch(*c)
            {
            case 'l':
                l = true;
                break;
            case 'a':
                a = true;
                break;
            default:
                fprintf(stderr, "ls: invalid argument \'%c\'\n", *c);
                exit(2);
            }
        }
    }

    DIR* dp;
    struct dirent* ep;     
    dp = opendir("./");
    if (dp != NULL)
    {
        while ((ep = readdir(dp)) != NULL)
        {
            if (!a && ep->d_name[0] == '.')
                continue;
            char* full_path = realpath(ep->d_name, NULL);
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
            free(full_path);
            if (dir)
                printf("\x1b[94m");
            else
                printf("\x1b[92m");
            printf("%s\x1b[0m\n", ep->d_name);
        }

        closedir(dp);
        return 0;
    }
    else
    {
        int _errno = errno;
        char cwd[PATH_MAX] = {0};
        getcwd(cwd, PATH_MAX);
        char buffer[256] = {0};
        snprintf(buffer, 256, "ls: cannot access \'%s\'", cwd);
        errno = _errno;
        perror(buffer);
        return -1;
    }
}