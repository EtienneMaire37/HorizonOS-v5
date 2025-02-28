#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

int main()
{
    // printnumber(time(NULL));
    // putchar('\n');
    // uint64_t i0 = 0;
    // uint64_t i1 = 1;
    // for (uint8_t i = 0; i < 15; i++)
    // {
    //     // printf("%llu\n", i0);
    //     printnumber(i0);
    //     uint64_t i2 = i0 + i1;
    //     i0 = i1;
    //     i1 = i2;
    //     putchar('\n');
    // }
    // srand(time(NULL));
    // for (uint8_t i = 0; i < 5; i++)
    // {
    //     printnumber(rand());
    //     putchar('\n');
    // }
    uint64_t n = 12345678234567890;
    printf("Hello, World! %lu %u %d\n", n, 52, -63);
    return 0;
}