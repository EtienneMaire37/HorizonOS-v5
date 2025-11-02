// #include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "../libc/include/stdint.h"
#include "../libc/include/stddef.h"

#include <bootboot.h>

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

int64_t minint(int64_t a, int64_t b);
int64_t maxint(int64_t a, int64_t b);
int64_t absint(int64_t x);
int imod(int a, int b);

#include "../libc/include/inttypes.h"
#include "../libc/include/limits.h"

#define hex_char_to_int(ch) (ch >= '0' && ch <= '9' ? (ch - '0') : (ch >= 'a' && ch <= 'f' ? (ch - 'a' + 10) : 0))

#define bcd_to_binary(bcd) (((bcd) & 0x0f) + (((bcd) >> 4) & 0x0f) * 10 + (((bcd) >> 8) & 0x0f) * 100 + (((bcd) >> 12) & 0x0f) * 1000)

char cwd[PATH_MAX] = {0};

char** environ = NULL;
static int num_environ = 0;

int64_t system_seconds = 0, system_minutes = 0, system_hours = 0, system_day = 0, system_month = 0;
int64_t system_year = 0;
int64_t system_thousands = 0;

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
#include "debug/out.h"

#include "../libc/include/errno.h"
#include "../libc/include/stdio.h"
#include "../libc/include/stdlib.h"
#include "../libc/src/string.c"
#include "../libc/include/string.h"
#include "../libc/include/unistd.h"
#include "../libc/include/termios.h"
#include "../libc/include/assert.h"
#include "../libc/include/sys/types.h"
#include "../libc/include/sys/wait.h"
#include "../libc/include/sys/stat.h"
#include "../libc/include/fcntl.h"
#include "../libc/include/dirent.h"

#include "vfs/vfs.h"
#include "time/ktime.h"
#include "initrd/initrd.h"
#include "time/gdn.h"
#include "time/ktime.h"
#include "time/time.h"
#include "cmos/rtc.h"

#include "vga/textio.c"
#include "pic/apic.c"
#include "fpu/fpu.c"

#include "../libc/src/kernel.c"
#include "../libc/src/stdio.c"
#include "../libc/src/stdlib.c"
#include "../libc/src/unistd.c"
#include "../libc/src/time.c"
#include "gdt/gdt.c"
#include "int/idt.c"
#include "int/int.c"
#include "memalloc/page_frame_allocator.c"
#include "paging/paging.c"
#include "multitasking/task.c"
#include "io/keyboard.c"
#include "ps2/keyboard.c"
#include "ps2/ps2.c"

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
int imod(int a, int b)
{
    if (b <= 0) 
    {
        LOG(WARNING, "Kernel tried to compute %d %% %d", a, b);
        return 0;
    }
    int ret = a - (a / b) * b;
    if (ret < 0) ret += b;
    return ret;
}

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
    uint8_t cpu_id = apic_get_cpu_id();

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

        LOG(INFO, "LAPIC base: %p", lapic);

        uint32_t ebx, ecx, edx;
        cpuid(0, cpuid_highest_function_parameter, ebx, ecx, edx);
        // !! Actually will cause a triple fault on CPUs that don't support CPUID but you shouldn't be running this OS on such hardware anyway

        LOG(INFO, "CPUID highest function parameter: 0x%x", cpuid_highest_function_parameter);

        *(uint32_t*)&manufacturer_id_string[0] = ebx;
        *(uint32_t*)&manufacturer_id_string[4] = edx;
        *(uint32_t*)&manufacturer_id_string[8] = ecx;
        manufacturer_id_string[12] = 0;

        LOG(INFO, "CPU manufacturer id : \"%s\"", manufacturer_id_string);

        cpuid(0x80000000, cpuid_highest_extended_function_parameter, ebx, ecx, edx);
        LOG(INFO, "CPUID highest extended function parameter: 0x%x", cpuid_highest_extended_function_parameter);

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

        init_pat();

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

// * vvv Now we can use printf

    tty_clear_screen(' ');

    printf("LAPIC base: %p\n", lapic);

    printf("CPU manufacturer id : ");
    tty_set_color(FG_LIGHTRED, BG_BLACK);
    printf("\"%s\"\n", manufacturer_id_string);
    tty_set_color(FG_WHITE, BG_BLACK);

    printf("CPUID highest function parameter: 0x%x\n", cpuid_highest_function_parameter);
    printf("CPUID highest extended function parameter: 0x%x\n", cpuid_highest_extended_function_parameter);

    printf("Physical address is ");
    tty_set_color(FG_LIGHTBLUE, BG_BLACK);
    printf("%u ", physical_address_width);
    tty_set_color(FG_WHITE, BG_BLACK);
    printf("bits long\n");

    pfa_detect_usable_memory();

    printf("Detected ");
    tty_set_color(FG_LIGHTBLUE, BG_BLACK);
    printf("%u ", allocatable_memory);
    tty_set_color(FG_WHITE, BG_BLACK);
    printf("bytes of allocatable memory\n");

    LOG(INFO, "Loading a GDT with TSS...");
    printf("Loading a GDT with TSS...");
    fflush(stdout);

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
    fflush(stdout);
    LOG(INFO, "Loading an IDT...");
    install_idt();
    printf(" | Done\n");
    LOG(INFO, "IDT loaded");

    LOG(INFO, "Disabling the PIC");
    printf("Disabling the PIC...");
    fflush(stdout);
    pic_disable();
    printf(" | Done\n");
    LOG(INFO, "PIC disabled");

    LOG(INFO, "Enabling the APIC");
    printf("Enabling the APIC...");
    fflush(stdout);

    lapic_set_spurious_interrupt_number(0xff);
    lapic_enable();
    lapic_set_tpr(0);

    printf(" | Done\n");
    LOG(INFO, "APIC enabled");

    LOG(INFO, "Setting up the APIC timer");
    printf("Setting up the APIC timer");
    fflush(stdout);

    {
        rtc_wait_while_updating();

        lapic->divide_configuration_register = 3;
        lapic->initial_count_register = 0xffffffff;

        rtc_get_time();

        lapic->lvt_timer_register = 0x10000;    // mask it

        uint32_t ticks_in_1_sec = 0xffffffff - lapic->current_count_register;

        lapic->lvt_timer_register = 0x80 | 0x20000; // 0x80 | PERIODIC
        lapic->divide_configuration_register = 3;
        lapic->initial_count_register = ticks_in_1_sec / GLOBAL_TIMER_FREQUENCY;

        time_initialized = true;
    }

    LOG(INFO, "Set up the APIC timer");
    printf(" | Done\n");

    printf("Time: ");

    tty_set_color(FG_LIGHTCYAN, BG_BLACK);
    printf("%u-%u-%u %u:%u:%u\n", system_year, system_month, system_day, system_hours, system_minutes, system_seconds);
    tty_set_color(FG_WHITE, BG_BLACK);

    enable_interrupts(); 
    LOG(INFO, "Enabled interrupts");

    if (pat_enabled)
    {
        LOG(INFO, "PAT successfully enabled");
        printf("PAT successfully enabled\n");
    }
    else
    {
        LOG(WARNING, "PAT not supported");
        printf("warning: PAT not supported (this might cause poor performance only graphical intensive programs)\n");
    }

    LOG(INFO, "Setting up paging...");
    printf("Setting up paging...\n");

    uint64_t* cr3 = create_empty_virtual_address_space();

    // LOG(DEBUG, "%s", bootboot.arch.x86_64.acpi_ptr);

    precise_time_t paging_start_time = global_timer;

    {
        uint64_t* bootboot_cr3 = (uint64_t*)get_cr3();

    // * "When the kernel gains control, the memory mapping looks like this:"
    // *  -128M         "mmio" area           (0xFFFFFFFFF8000000)
    // *   -64M         "fb" framebuffer      (0xFFFFFFFFFC000000)
    // *    -2M         "bootboot" structure  (0xFFFFFFFFFFE00000)
    // *    -2M+1page   "environment" string  (0xFFFFFFFFFFE01000)
    // *    -2M+2page.. code segment   v      (0xFFFFFFFFFFE02000)
    // *     ..0        stack          ^      (0x0000000000000000)
    // *    0-16G       RAM identity mapped   (0x0000000400000000)

        printf("Copying mapping of range 0x%x-0x%x from bootboot\n", 0xFFFFFFFFFFE00000, 0);

        LOG(DEBUG, "Copying mapping of range 0x%x-0x%x from bootboot", 0xFFFFFFFFFFE00000, 0);
        copy_mapping(bootboot_cr3, cr3, 0xFFFFFFFFFFE00000, (-0xFFFFFFFFFFE00000) >> 12);

        for (MMapEnt* mmap_ent = &bootboot.mmap; (uintptr_t)mmap_ent < (uintptr_t)&bootboot + (uintptr_t)bootboot.size; mmap_ent++)
        {
            uint64_t ptr = MMapEnt_Ptr(mmap_ent) & 0xfffffffffffff000;
            uint64_t len = ((MMapEnt_Size(mmap_ent) + 0xfff) / 0x1000) * 0x1000 + 0x1000;

            if (!MMapEnt_IsFree(mmap_ent))
                continue;

            if (ptr >= 1 * TB)
                continue;
            if (ptr + len >= 1 * TB)
                len = 1 * TB - ptr;
                
            LOG(DEBUG, "Identity mapping range 0x%x-0x%x", ptr, ptr + len);
            printf("Identity mapping range 0x%x-0x%x\n", ptr, ptr + len);
            remap_range(cr3, ptr, ptr, len >> 12, PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);
        }

        printf("Identity mapping range 0x%x-0x%x\n", lapic, (uint64_t)lapic + 0x1000);
        LOG(DEBUG, "Identity mapping range 0x%x-0x%x", lapic, (uint64_t)lapic + 0x1000);
        remap_range(cr3, (uint64_t)lapic, (uint64_t)lapic, 1, PG_SUPERVISOR, PG_READ_WRITE, CACHE_WT);

        printf("Identity mapping range 0x%x-0x%x\n", framebuffer.address, ((framebuffer.address + framebuffer.stride * framebuffer.height + 0xfff) / 0x1000) * 0x1000);
        LOG(DEBUG, "Identity mapping range 0x%x-0x%x", framebuffer.address, ((framebuffer.address + framebuffer.stride * framebuffer.height + 0xfff) / 0x1000) * 0x1000);
        remap_range(cr3, (uint64_t)framebuffer.address, (uint64_t)framebuffer.address, (framebuffer.stride * framebuffer.height + 0xfff) / 0x1000, PG_SUPERVISOR, PG_READ_WRITE, CACHE_WC);

        printf("Identity mapping range 0x%x-0x%x\n", bootboot.initrd_ptr & 0xfffffffffffff000, (bootboot.initrd_ptr & 0xfffffffffffff000) + ((bootboot.initrd_size + 0x1fff) / 0x1000) * 0x1000);
        LOG(DEBUG, "Identity mapping range 0x%x-0x%x", bootboot.initrd_ptr & 0xfffffffffffff000, (bootboot.initrd_ptr & 0xfffffffffffff000) + ((bootboot.initrd_size + 0x1fff) / 0x1000) * 0x1000);
        remap_range(cr3, (uint64_t)bootboot.initrd_ptr & 0xfffffffffffff000, (uint64_t)bootboot.initrd_ptr & 0xfffffffffffff000, (bootboot.initrd_size + 0x1fff) / 0x1000, PG_SUPERVISOR, PG_READ_WRITE, CACHE_WB);
    }

    uint32_t paging_milliseconds = precise_time_to_milliseconds(global_timer - paging_start_time);

    printf("Paging setup done in %u.%u%u%u seconds\n", paging_milliseconds / 1000, (paging_milliseconds / 100) % 10, (paging_milliseconds / 10) % 10, paging_milliseconds % 10);
    LOG(INFO, "Set up paging");

    load_cr3((uint64_t)cr3);

    // LOG(DEBUG, "%s", bootboot.arch.x86_64.acpi_ptr); // TODO: Add proper UEFI style ACPI parsing and map the tables

    // asm volatile("div rcx" :: "c"(0));

    fflush(stdout);

    while(true)
        hlt();

    halt();
}