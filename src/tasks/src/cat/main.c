#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

int main(int argc, char** argv)
{
    char data[BUFSIZ];
    for (int i = 1; i < argc; i++)
    {
        struct stat st;
        if (stat(argv[i], &st) == 0 && S_ISDIR(st.st_mode))
        {
            fprintf(stderr, "cat: %s: Is a directory\n", argv[i]);
            continue;
        }

        FILE* f = fopen(argv[i], "rb");
        if (!f)
        {
            int _errno = errno;
            fprintf(stderr, "cat: %s: ", argv[i]);
            errno = _errno;
            perror("");
            continue;
        }

        size_t bytes_read;
        while ((bytes_read = fread(data, 1, BUFSIZ, f)) > 0)
            fwrite(data, 1, bytes_read, stdout);

        fclose(f);
    }
}