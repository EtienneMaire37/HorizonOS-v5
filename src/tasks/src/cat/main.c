#include <stdio.h>
#include <limits.h>

extern int errno;

int main(int argc, char** argv)
{
    char data[BUFSIZ];
    for (int i = 1; i < argc; i++)
    {
        FILE* f = fopen(argv[i], "rb");
        if (!f)
        {
            int _errno = errno;
            char buf[PATH_MAX + 5];
            snprintf(buf, PATH_MAX + 5, "cat: %s", argv[i]);
            errno = _errno;
            perror(buf);
            return 1;
        }
        while (fread(data, BUFSIZ, 1, f) == 1)
            printf("%s", data);
    }
}