// #include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "../libc/include/stdint.h"
#include "../libc/include/stddef.h"

#include <bootboot.h>

uint8_t physical_address_width; // ! Should probably be in paging.h but didn't add it back yet

typedef uint64_t physical_address_t;
typedef uint64_t virtual_address_t;

extern BOOTBOOT bootboot;
extern uint8_t environment[4096];
extern uint8_t fb;

extern char kernel_start, kernel_end;
physical_address_t kernel_start_phys, kernel_end_phys;

#define enable_interrupts()     asm volatile ("sti")
#define disable_interrupts()    asm volatile ("cli")

#define hlt()                   asm volatile ("hlt")

void halt();

void _halt()
{
    while (true)
    {
        disable_interrupts();
        hlt();
    }
    __builtin_unreachable();
}

// * Only support 48-bit canonical addresses for now (4 level paging)
bool is_address_canonical(uint64_t address)
{
    return (address < 0x0000800000000000ULL) || (address >= 0xffff800000000000ULL);
}
uint64_t make_address_canonical(uint64_t address)
{
    if (address & 0x0000800000000000ULL)
        return address | 0xffff800000000000ULL;
    else
        return address & 0x00007fffffffffffULL;
}

#define KB 1024ULL
#define MB (1024ULL * KB)
#define GB (1024ULL * MB)
#define TB (1024ULL * GB)

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

#define hex_char_to_int(ch) (ch >= '0' && ch <= '9' ? (ch - '0') : (ch >= 'a' && ch <= 'f' ? (ch - 'a' + 10) : 0))

#define bcd_to_binary(bcd) (((bcd) & 0x0f) + ((bcd) >> 4) * 10)

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
#include "cpu/cpuid.h"
#include "fpu/sse.h"

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
#include "../libc/include/fcntl.h"
#include "../libc/include/dirent.h"

#include "debug/out.h"
#include "vfs/vfs.h"
#include "time/ktime.h"
#include "initrd/initrd.h"

#include "vga/textio.c"
#include "pic/apic.c"
#include "fpu/fpu.c"

#include "../libc/src/kernel.c"
#include "../libc/src/stdio.c"
#include "../libc/src/stdlib.c"
#include "../libc/src/string.c"
#include "../libc/src/unistd.c"
#include "../libc/src/time.c"
#include "gdt/gdt.c"
#include "int/idt.c"
#include "int/int.c"
#include "memalloc/page_frame_allocator.c"

void cause_halt(const char* func, const char* file, int line)
{
    disable_interrupts();
    // LOG(ERROR, "Kernel halted in function \"%s\" at line %d in file \"%s\"", func, line, file);
    _halt();
}

void simple_cause_halt()
{
    disable_interrupts();
    // LOG(ERROR, "Kernel halted");
    _halt();
}

void halt()
{
    // fflush(stdout);
    cause_halt(__CURRENT_FUNC__, __FILE__, __LINE__);
}

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
    disable_interrupts();

    enable_sse();

    if (!bootboot.fb_scanline) 
        abort();

    apic_init();
    uint16_t cpu_id = apic_get_cpu_id();

    if (bootboot.bspid != cpu_id) // * Only one core supported for now
        halt();

    if (bootboot.bspid == cpu_id)
    {
        kernel_init_std();

        kernel_start_phys = (physical_address_t)&kernel_start;
        kernel_end_phys = (physical_address_t)&kernel_end;

        LOG(INFO, "Kernel booted successfully with BOOTBOOT (0x%x-0x%x)", kernel_start_phys, kernel_end_phys);
        LOG(INFO, "Kernel is %u bytes long", kernel_end_phys - kernel_start_phys);
        LOG(INFO, "Framebuffer : (%u, %u) (scanline %u bytes) at 0x%x", bootboot.fb_width, bootboot.fb_height, bootboot.fb_scanline, bootboot.fb_ptr);
        LOG(INFO, "Type: %s", fb_type_string[bootboot.fb_type]);

        framebuffer.width = bootboot.fb_width;
        framebuffer.height = bootboot.fb_height;
        framebuffer.stride = bootboot.fb_scanline;
        framebuffer.address = (uintptr_t)bootboot.fb_ptr;
        framebuffer.format = bootboot.fb_type;

        tty_cursor = 0;

        has_cpuid = true;
        uint32_t ebx, ecx, edx;
        cpuid(0, cpuid_highest_function_parameter, ebx, ecx, edx);
        // !! Actually will cause a triple fault on CPUs that don't support CPUID but you shouldn't be running this OS on such hardware anyway

        LOG(DEBUG, "CPUID highest function parameter: 0x%x", cpuid_highest_function_parameter);

        *(uint32_t*)&manufacturer_id_string[0] = ebx;
        *(uint32_t*)&manufacturer_id_string[4] = edx;
        *(uint32_t*)&manufacturer_id_string[8] = ecx;
        manufacturer_id_string[12] = 0;

        LOG(INFO, "cpu manufacturer id : \"%s\"", manufacturer_id_string);

        cpuid(0x80000000, cpuid_highest_extended_function_parameter, ebx, ecx, edx);
        LOG(DEBUG, "CPUID highest extended function parameter: 0x%x", cpuid_highest_extended_function_parameter);

        if (cpuid_highest_extended_function_parameter >= 0x80000008)
        {
            uint32_t eax = 0, ebx, ecx, edx; // Just so gcc doesn't complain but cant possibly be used uninitialized
            cpuid(0x80000008, eax, ebx, ecx, edx);
            physical_address_width = eax & 0xff;
        }
        else
            abort();

        LOG(INFO, "Physical address is %u bits long", physical_address_width);

        fpu_init_defaults();

        atomic_store(&did_init_std, true);
    }

    while (!atomic_load(&did_init_std));

    acquire_spinlock(&print_spinlock);

    LOG(INFO, "cpu_id : %u", cpu_id);

    {
        if (cpuid_highest_function_parameter >= 1)
        {
            uint32_t eax, ebx, ecx = 0, edx; // same as above
            cpuid(1, eax, ebx, ecx, edx);
            if (((ecx >> 26) & 1) && ((ecx >> 28) & 1)) // * AVX and XSAVE supported
            {
                LOG(INFO, "Enabling AVX");
                enable_avx();
            }
        }
    }

    release_spinlock(&print_spinlock);

    initrd_parse(bootboot.initrd_ptr, bootboot.initrd_ptr + bootboot.initrd_size);
    kernel_symbols_file = initrd_find_file("symbols.txt");

    tty_font = psf_font_load_from_initrd("iso06.f14.psf");

    printf("CPUID highest function parameter: 0x%x\n", cpuid_highest_function_parameter);
    printf("CPUID highest extended function parameter: 0x%x\n", cpuid_highest_extended_function_parameter);
    printf("Physical address is %u bits long\n", physical_address_width);

    LOG(INFO, "Loading a GDT with TSS...");
    printf("Loading a GDT with TSS...");

    memset(&GDT[0], 0, sizeof(struct gdt_entry));       // NULL Descriptor
    setup_gdt_entry(&GDT[1], 0, 0xfffff, 0x9A, 0xA);    // Kernel mode code segment
    setup_gdt_entry(&GDT[2], 0, 0xfffff, 0x92, 0xC);    // Kernel mode data segment
    setup_gdt_entry(&GDT[3], 0, 0xfffff, 0xFA, 0xA);    // User mode code segment
    setup_gdt_entry(&GDT[4], 0, 0xfffff, 0xF2, 0xC);    // User mode data segment

    memset(&TSS, 0, sizeof(struct tss_entry));
    TSS.rsp0 = 0;
    setup_ssd_gdt_entry(&GDT[5], (physical_address_t)&TSS, sizeof(struct tss_entry) - 1, 0x89, 0);  // TSS

    install_gdt();
    load_tss();

    printf(" | Done\n");
    LOG(INFO, "GDT and TSS loaded");

    printf("Loading an IDT...");
    LOG(INFO, "Loading an IDT...");
    install_idt();
    printf(" | Done\n");
    LOG(INFO, "IDT loaded");

    LOG(INFO, "Remapping the PIC");
    printf("Remapping the PIC...");
    pic_remap(32, 32 + 8);
    printf(" | Done\n");
    LOG(INFO, "PIC remapped");

    enable_interrupts(); 
    LOG(INFO, "Enabled interrupts");

    pfa_detect_usable_memory();

    printf("Detected %u bytes of allocatable memory\n", allocatable_memory);

    fflush(stdout);

    halt();
}