#include <stddef.h>
#include <stdint.h>

#include "../cpu/util.h"
#include "../cpu/registers.h"
#include "../debug/out.h"
#include "../panic/panic.h"

void __attribute__((noreturn)) abort()
{
    disable_interrupts();
    LOG(ERROR, "Kernel aborted.");
    print_stack_trace((uint64_t)abort, get_rbp(), false);
    printf("\x1b[31mKernel aborted.\x1b[0m\n");
    halt();
}

int atexit(void (*func)())
{
    (void)func;
    return 0;
}
