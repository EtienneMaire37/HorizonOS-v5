#include <unistd.h>
#include <string.h>

int main()
{
    const char* str = "\x1b[2J\x1b[H";
    write(STDOUT_FILENO, str, strlen(str));
}