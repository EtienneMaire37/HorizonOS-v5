#include <stdbool.h>
#include "../../libc/include/inttypes.h"

#include "../../libc/include/math.h"
#include "../../libc/include/stdio.h"
#include "../../libc/include/stdlib.h"
#include "../../libc/include/syscall_defines.h"

void kmain()
{
    // asm volatile ("int 0xf0" :: "a" (SYSCALL_EXIT), "b" (0));
    long double res = 1;
    for (int i = 3; true; i += 4)
    {
        long double inc = 1.L / (i + 2) - 1.L / i;
        if (isinf(inc) || isnan(inc)) break;
        if (isinf(res) || isnan(res)) break;

        res += inc;

        if (((i - 3) / 4) % 1000 == 0)
        {
            // printf("pi ~= %lf\n", 4 * res);
            printf("pi ~= 3.");
            uint64_t mul = 10;
            for (int j = 0; j < 18; j++)
            {
                putchar('0' + ((uint64_t)(4 * res * mul) % 10));
                mul *= 10;
            }
            putchar('\n');
        }
    }
    exit(0);
}