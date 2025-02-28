#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

void printnumber(uint64_t n)
{
    uint64_t div = 10000000000000000000;
    bool first0 = true;
    while (div > 0)
    {
        uint8_t digit = (n / div) % 10;
        if (digit || div == 1)
            first0 = false;
        if (!first0)
            putchar('0' + digit);
        div /= 10;
    }
}

int main()
{
    printnumber(time(NULL));
    putchar('\n');
    uint64_t i0 = 0;
    uint64_t i1 = 1;
    for (uint8_t i = 0; i < 15; i++)
    {
        // printf("%llu\n", i0);
        printnumber(i0);
        uint64_t i2 = i0 + i1;
        i0 = i1;
        i1 = i2;
        putchar('\n');
    }
    srand(time(NULL));
    for (uint8_t i = 0; i < 5; i++)
    {
        printnumber(rand());
        putchar('\n');
    }
    return 0;
}