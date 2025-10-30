// #include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "../libc/include/stdint.h"
#include "../libc/include/stddef.h"

#include <bootboot.h>

extern BOOTBOOT bootboot;
extern uint8_t environment[4096];
extern uint8_t fb;

#define enable_interrupts()     asm volatile ("sti")
#define disable_interrupts()    asm volatile ("cli")

#define hlt()                   asm volatile ("hlt")

void halt()
{
    while (1) 
    {
        disable_interrupts();
        hlt();
    }
    __builtin_unreachable();
}

#define KB 1024
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

#define BUILDING_C_LIB
#define BUILDING_KERNEL

#ifndef __CURRENT_FUNC__

#if __STDC_VERSION__ >= 199901L
#define __CURRENT_FUNC__    __func__
#elif __GNUC__ >= 2
#define __CURRENT_FUNC__    __FUNCTION__
#else
#define __CURRENT_FUNC__    ""
#endif

#endif

typedef uint64_t physical_address_t;
typedef uint64_t virtual_address_t;

#define physical_null ((physical_address_t)0)

#include "../libc/include/inttypes.h"
#include "../libc/include/limits.h"

int64_t minint(int64_t a, int64_t b)
{
    return a < b ? a : b;
}
int64_t maxint(int64_t a, int64_t b)
{
    return a > b ? a : b;
}
int64_t absint(int64_t x)
{
    return x < 0 ? -x : x;
}

char cwd[PATH_MAX] = {0};

char** environ = NULL;
static int num_environ = 0;

uint8_t system_seconds = 0, system_minutes = 0, system_hours = 0, system_day = 0, system_month = 0;
uint16_t system_year = 0;
uint16_t system_thousands = 0;

bool time_initialized = false;

#include "io/io.h"
#include "cpu/cpuid.h"
#include "cpu/msr.h"
#include "cpu/registers.h"
#include "multicore/spinlock.h"
#include "../libc/src/misc.h"
#include "graphics/linear_framebuffer.h"

#include "../libc/include/errno.h"
#include "../libc/include/stdio.h"
#include "../libc/include/stdlib.h"
#include "../libc/include/string.h"
#include "../libc/include/unistd.h"
#include "../libc/include/termios.h"
#include "../libc/include/assert.h"
#include "../libc/include/sys/types.h"
#include "../libc/include/sys/wait.h"
#include "../libc/include/sys/stat.h"

#include "debug/out.h"
#include "time/ktime.h"
#include "initrd/initrd.h"

#include "vga/textio.c"
#include "pic/apic.c"

#include "../libc/src/kernel.c"
#include "../libc/src/stdio.c"
#include "../libc/src/stdlib.c"
#include "../libc/src/string.c"
#include "../libc/src/unistd.c"
#include "../libc/src/time.c"

FILE _stdin, _stdout, _stderr;

uint8_t stdin_buffer[BUFSIZ];
uint8_t stdout_buffer[BUFSIZ];
uint8_t stderr_buffer[BUFSIZ];

#define _init_file_flags(f) { f->fd = -1; f->buffer_size = BUFSIZ; f->buffer_index = 0; f->buffer_mode = 0; f->flags = FILE_FLAGS_BF_ALLOC; f->current_flags = 0; f->buffer_end_index = 0;}

void kernel_init_std()
{
    stdin = &_stdin;
    stdout = &_stdout;
    stderr = &_stderr;

    stdin->buffer = stdin_buffer;
    stdout->buffer = stdout_buffer;
    stderr->buffer = stderr_buffer;

    _init_file_flags(stdin);
    _init_file_flags(stdout);
    _init_file_flags(stderr);
    
    stdin->fd = STDIN_FILENO;
    stdin->flags = FILE_FLAGS_READ;

    stdout->fd = STDOUT_FILENO;
    stdout->flags = FILE_FLAGS_WRITE | FILE_FLAGS_LBF;

    stderr->fd = STDERR_FILENO;
    stderr->flags = FILE_FLAGS_WRITE | FILE_FLAGS_NBF;
}

int _gdtr, _idtr;

void interrupt_handler()
{
    ;
}

atomic_flag print_spinlock = ATOMIC_FLAG_INIT;
atomic_bool did_init_std = false;

const char* fb_type_string[] = 
{
    "FB_ARGB",
    "FB_RGBA",
    "FB_ABGR",
    "FB_BGRA"
};

void _start()
{
    if (!bootboot.fb_scanline) 
        abort();

    apic_init();
    uint16_t cpu_id = apic_get_cpu_id();

    if (bootboot.bspid != cpu_id) // * Only one core supported for now
        halt();

    if (bootboot.bspid == cpu_id)
    {
        kernel_init_std();

        LOG(INFO, "Kernel booted successfully with BOOTBOOT");
        LOG(INFO, "Framebuffer : (%u, %u) (scanline %u bytes) at 0x%x", bootboot.fb_width, bootboot.fb_height, bootboot.fb_scanline, bootboot.fb_ptr);
        LOG(INFO, "Type: %s", fb_type_string[bootboot.fb_type]);

        framebuffer.width = bootboot.fb_width;
        framebuffer.height = bootboot.fb_height;
        framebuffer.stride = bootboot.fb_scanline;
        framebuffer.address = (uintptr_t)bootboot.fb_ptr;
        framebuffer.format = bootboot.fb_type;

        atomic_store(&did_init_std, true);
    }

    while (!atomic_load(&did_init_std));

    acquire_spinlock(&print_spinlock);
    LOG(INFO, "cpu_id : %u", cpu_id);
    release_spinlock(&print_spinlock);

    initrd_parse(bootboot.initrd_ptr, bootboot.initrd_ptr + bootboot.initrd_size);

    tty_font = psf_font_load_from_initrd("iso06.f14.psf");

    printf("Hello from kernel!\n");
    printf("Hey this is a newline\n\n");
    for (int i = 0; i < 1000; i++)
        printf("%c%c%c", 'A' + (i % 26), 'A' + (i % 26), 'A' + (i % 26));

    fflush(stdout);

    halt();
}