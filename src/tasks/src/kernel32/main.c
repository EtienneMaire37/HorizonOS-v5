#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
// #include <stdint.h>

#define BUF_LEN 5

int main()
{
    char c[BUF_LEN];
    while (true)
    {
        uint8_t bytes_read = read(STDIN_FILENO, c, BUF_LEN);
        /*for (uint8_t i = 0; i < bytes_read; i++)
            putchar(c[i]);*/
        printf("%d\n", bytes_read);
    }
}