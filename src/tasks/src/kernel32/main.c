#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

void func1()
{
    printf("Function 1 called\n");
}

void func2()
{
    printf("Function 2 called\n");
}

int main()
{
    // printf("%u\n", time(NULL));
    // uint64_t i0 = 0;
    // uint64_t i1 = 1;
    // for (uint8_t i = 0; i < 15; i++)
    // {
    //     printf("%lu\n", i0);
    //     uint64_t i2 = i0 + i1;
    //     i0 = i1;
    //     i1 = i2;
    // }
    // srand(time(NULL));
    // for (uint8_t i = 0; i < 5; i++)
    //     printf("%u\n", rand());

    atexit(func1);
    atexit(func2);
    printf("%ld\n", a64l("BDz5/"));
    printf("%s\n", l64a(1382));
    return 0;
}