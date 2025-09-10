#include <stdbool.h>
#include "../../libc/include/inttypes.h"

#include "../../libc/include/math.h"
#include "../../libc/include/stdio.h"
#include "../../libc/include/stdlib.h"
#include "../../libc/include/syscall_defines.h"

#include "../../libc/include/unistd.h"

void kmain()
{
    asm volatile ("int 0xf0" :: "a" (SYSCALL_WRITE), "b" (STDOUT_FILENO), "c" ("Hello World!\n"), "d" (13));
    asm volatile ("int 0xf0" :: "a" (SYSCALL_EXIT), "b" (0));
}