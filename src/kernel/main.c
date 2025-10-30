// #include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "../libc/include/stdint.h"
#include "../libc/include/stddef.h"

#include <bootboot.h>

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

#include "vga/textio.c"
#include "pic/apic.c"

#include "../libc/src/kernel.c"
#include "../libc/src/stdio.c"
#include "../libc/src/stdlib.c"
#include "../libc/src/string.c"
#include "../libc/src/unistd.c"
#include "../libc/src/time.c"

int _gdtr, _idtr;

void interrupt_handler()
{
    ;
}

extern BOOTBOOT bootboot;
extern unsigned char environment[4096];
extern uint8_t fb;

void _start()
{
    if (!bootboot.fb_scanline) 
        abort();

    apic_init();
    uint16_t cpu_id = apic_get_cpu_id();

    if (bootboot.bspid != cpu_id) // * Only one core supported for now
        halt();

    const char* str = "Kernel booted successfully with BOOTBOOT";
    size_t len = strlen(str);
    write(STDERR_FILENO, str, len);

    // puts("Hello from a simple BOOTBOOT kernel");

    // LOG(INFO, "Kernel booted successfully with BOOTBOOT");
    
    halt();
}

// typedef struct 
// {
//     uint32_t magic;
//     uint32_t version;
//     uint32_t headersize;
//     uint32_t flags;
//     uint32_t numglyph;
//     uint32_t bytesperglyph;
//     uint32_t height;
//     uint32_t width;
//     uint8_t glyphs;
// } __attribute__((packed)) psf2_t;

// extern volatile psf2_t _binary_resources_font_psf_start;

// void puts(char* s)
// {
//     int x,y,kx=0,line,mask,offs;
//     int bpl=(_binary_resources_font_psf_start.width+7)/8;
//     while (*s) 
//     {
//         uint8_t* glyph = (uint8_t*)&_binary_resources_font_psf_start + _binary_resources_font_psf_start.headersize + (*s > 0 && *s < _binary_resources_font_psf_start.numglyph ? *s : 0) * _binary_resources_font_psf_start.bytesperglyph;
//         offs = (kx * (_binary_resources_font_psf_start.width+1) * 4);
//         for(y=0;y<_binary_resources_font_psf_start.height;y++) {
//             line=offs; mask=1<<(_binary_resources_font_psf_start.width-1);
//             for(x=0;x<_binary_resources_font_psf_start.width;x++) {
//                 *((uint32_t*)((uint64_t)&fb+line))=((int)*glyph) & (mask)?0xFFFFFF:0;
//                 mask>>=1; line+=4;
//             }
//             *((uint32_t*)((uint64_t)&fb+line))=0; glyph+=bpl; offs+=bootboot.fb_scanline;
//         }
//         s++; kx++;
//     }
// }
