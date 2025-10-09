#include <stdio.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
// #include <dirent.h>

extern int errno;

int main()
{
    // DIR* dp;
    // struct dirent* ep;     
    // dp = opendir("./");
    // if (dp != NULL)
    // {
    //     while ((ep = readdir(dp)) != NULL)
    //         puts(ep->d_name);

    //     closedir(dp);
    //     return 0;
    // }
    // else
    {
        int _errno = errno;
        char cwd[PATH_MAX] = {0};
        getcwd(cwd, PATH_MAX);
        char buffer[256] = {0};
        snprintf(buffer, 256, "cannot access \'%s\'", cwd);
        errno = _errno;
        perror(buffer);
        return -1;
    }
}