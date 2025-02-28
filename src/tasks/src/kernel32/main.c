#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

int main()
{
    printf("%u\n", time(NULL));
    uint64_t i0 = 0;
    uint64_t i1 = 1;
    for (uint8_t i = 0; i < 15; i++)
    {
        printf("%lu\n", i0);
        uint64_t i2 = i0 + i1;
        i0 = i1;
        i1 = i2;
    }
    srand(time(NULL));
    for (uint8_t i = 0; i < 5; i++)
        printf("%u\n", rand());
    return 0;
}