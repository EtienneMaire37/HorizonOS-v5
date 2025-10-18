#include <stdio.h>

extern char** environ;

int main()
{
    if (!environ) return 0;
    for (int i = 0; environ[i]; i++)
        puts(environ[i]);
}