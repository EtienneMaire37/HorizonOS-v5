// #include <stddef.h>
// #include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdatomic.h>

#include "multiboot.h"
multiboot_info_t* multiboot_info;

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

#include "../libc/include/assert.h"

#include "../libc/include/errno.h"
#include "../libc/include/stddef.h"
#include "../libc/include/stdarg.h"
#include "../libc/include/unistd.h"
#include "../libc/include/sys/types.h"
#include "../libc/include/inttypes.h"
#include "../libc/include/string.h"
#include "../libc/include/time.h"

#include "../libc/src/kernel_glue.h"

const char* text_logo = 
// "    __  __           _                  ____  _____            ______ \n\
//    / / / /___  _____(_)___ ____  ____  / __ \\/ ___/     _   __/ ____/ \n\
//   / /_/ / __ \\/ ___/ /_  // __ \\/ __ \\/ / / /\\__ \\_____| | / /___ \\ \n\
//  / __  / /_/ / /  / / / // /_/ / / / / /_/ /___/ /_____/ |/ /___/ / \n\
// /_/ /_/\\____/_/  /_/ /___|____/_/ /_/\\____//____/      |___/_____/\n";
"  _    _            _                 ____   _____             _____ \n\
 | |  | |          (_)               / __ \\ / ____|           | ____| \n\
 | |__| | ___  _ __ _ _______  _ __ | |  | | (___ ________   _| |__   \n\
 |  __  |/ _ \\| '__| |_  / _ \\| '_ \\| |  | |\\___ \\______\\ \\ / /___ \\  \n\
 | |  | | (_) | |  | |/ / (_) | | | | |__| |____) |      \\ V / ___) | \n\
 |_|  |_|\\___/|_|  |_/___\\___/|_| |_|\\____/|_____/        \\_/ |____/";

int imod(int a, int b)
{
    return a - (a / b) * b;
}

typedef uint64_t physical_address_t;
typedef uint32_t virtual_address_t;

#define physical_null ((physical_address_t)0)

extern uint8_t stack_top;
extern uint8_t stack_bottom;

extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

virtual_address_t kernel_start;
virtual_address_t kernel_end;

uint64_t kernel_size = 0;
uint64_t available_memory = 0;

extern void _halt();
void cause_halt(const char* func, const char* file, int line);
void simple_cause_halt();

#define hlt()                   asm volatile ("hlt")

#define enable_interrupts()     asm volatile("sti")
#define disable_interrupts()    asm volatile("cli")

#define halt() (fflush(stdout), cause_halt(__CURRENT_FUNC__, __FILE__, __LINE__))

#define hex_char_to_int(ch) (ch >= '0' && ch <= '9' ? (ch - '0') : (ch >= 'a' && ch <= 'f' ? (ch - 'a' + 10) : 0))

int64_t minint(int64_t a, int64_t b)
{
    return a < b ? a : b;
}
int64_t maxint(int64_t a, int64_t b)
{
    return a > b ? a : b;
}

#define bcd_to_binary(bcd) (((bcd) & 0x0f) + ((bcd) >> 4) * 10)

physical_address_t virtual_address_to_physical(virtual_address_t address);
virtual_address_t physical_address_to_virtual(physical_address_t address);

multiboot_module_t* initrd_module;

const char* multiboot_block_type_text[5] = 
{
    "MULTIBOOT_MEMORY_AVAILABLE",
    "MULTIBOOT_MEMORY_RESERVED",
    "MULTIBOOT_MEMORY_ACPI_RECLAIMABLE",
    "MULTIBOOT_MEMORY_NVS",
    "MULTIBOOT_MEMORY_BADRAM"
};

// #include "klibc/arithmetic.c"
// #include "../libc/src/arithmetic.c"

// // // #include "../libc/src/math.c"

#include "multicore/spinlock.h"
#include "cpu/cpuid.h"
#include "cpu/registers.h"

#include "../libc/include/math.h"

#include "../libc/include/stdio.h"
#include "../libc/include/string.h"
#include "../libc/include/stdlib.h"
#include "../libc/src/stdio.c"
#include "../libc/src/string.c"
#include "../libc/src/stdlib.c"

#include "io/io.h"
#include "ps2/ps2.h"
#include "debug/out.h"
#include "fpu/fpu.h"

#include "io/keyboard.h"
#include "ps2/keyboard.h"
#include "acpi/tables.h"
#include "vga/textio.h"
#include "pci/pci.h"
#include "gdt/gdt.h"
#include "paging/paging.h"
#include "pit/pit.h"
#include "idt/idt.h"
#include "idt/int.h"
#include "pic/pic.h"
#include "pic/apic.h"
#include "multitasking/task.h"
#include "cmos/cmos.h"
#include "cmos/rtc.h"
#include "memalloc/page_frame_allocator.h"
#include "files/ustar.h"
#include "files/elf.h"
#include "initrd/initrd.h"
#include "time/gdn.h"
#include "time/ktime.h"

// ---------------------------------------------------------------

struct page_table_entry page_table_0[1024] __attribute__((aligned(4096)));
struct page_table_entry page_table_767[1024] __attribute__((aligned(4096)));
struct page_table_entry page_table_768_1023[256 * 1024] __attribute__((aligned(4096)));

// ---------------------------------------------------------------


#include "../libc/src/time.c"

#include "fpu/fpu.c"
#include "io/keyboard.c"
#include "ps2/keyboard.c"
#include "vga/vga.h"
#include "acpi/tables.c"
#include "memalloc/page_frame_allocator.c"
#include "vga/textio.c"
#include "gdt/gdt.c"
#include "paging/paging.c"
#include "pit/pit.c"
#include "idt/idt.c"
#include "idt/int.c"
#include "pic/pic.c"
#include "multitasking/task.c"
#include "multitasking/loader.h"
#include "ps2/ps2.c"
#include "pci/pci.c"

#include "../libc/src/kernel.c"

FILE _stdin, _stdout, _stderr;

uint8_t stdin_buffer[BUFSIZ];
uint8_t stdout_buffer[BUFSIZ];
uint8_t stderr_buffer[BUFSIZ];

#define cause_int_0() do { asm volatile("div ecx" :: "c" (0)); } while(0)

void cause_halt(const char* func, const char* file, int line)
{
    disable_interrupts();
    LOG(ERROR, "Kernel halted in function \"%s\" at line %d in file \"%s\"", func, line, file);
    _halt();
}

void simple_cause_halt()
{
    disable_interrupts();
    LOG(ERROR, "Kernel halted");
    _halt();
}

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

physical_address_t virtual_address_to_physical(virtual_address_t address)
{
    if (address < 0x100000) return (physical_address_t)address;
    if (address >= 0xc0000000 && address < (uint32_t)4 * 1024 * 1024 * 1023) return address - 0xc0000000;
    LOG(CRITICAL, "Unmapped virtual address 0x%x", address);
    abort();
    return 0;
}

virtual_address_t physical_address_to_virtual(physical_address_t address)
{
    if (address < 0x100000) return (virtual_address_t)address;
    if (address < (uint32_t)4 * 1024 * 1024 * 1023 - 0xc0000000) return address + 0xc0000000;
    LOG(CRITICAL, "Unmapped physical address 0x%lx", address);
    abort();
    return 0;
}

void __attribute__((cdecl)) kernel(multiboot_info_t* _multiboot_info, uint32_t magic_number)
{
    multiboot_info = _multiboot_info;
    tty_cursor = 0;

    current_phys_mem_page = 0xffffffff;
    kernel_size = &_kernel_end - &_kernel_start;

    current_cr3 = virtual_address_to_physical((virtual_address_t)page_directory);

    if (physical_memory_page_index < 0 || physical_memory_page_index >= 1024)       abort();
    if (kernel_stack_page_index < 0 || kernel_stack_page_index >= 1024)             abort();
    if (stack_page_index_start < 0 || stack_page_index_start >= 1024)               abort();
    if (stack_page_index_end < 0 || stack_page_index_end >= 1024)                   abort();

    kernel_init_std();

    vga_init();

    vga_write_port_3c0(0x10 | (1 << 5), vga_read_port_3c0(0x10 | (1 << 5)) & (~(1 << 3)));    // * Disable blinking

    tty_clear_screen(' ');
    tty_reset_cursor();

    LOG(INFO, "Kernel loaded at address 0x%x - 0x%x (%u bytes long)", &_kernel_start, &kernel_end, kernel_size); 
    LOG(INFO, "Stack : 0x%x-0x%x", &stack_bottom, &stack_top);

    uint32_t max_kernel_size = (uint32_t)(((uint32_t)4096 * 1023 * 1024) - (uint32_t)&_kernel_start);
    if (kernel_size >= max_kernel_size)
    {
        LOG(CRITICAL, "Kernel is too big (max %uB)", max_kernel_size); 
        printf("Kernel is too big (max %uB)\n", max_kernel_size);
        abort();
    }

    if(magic_number != MULTIBOOT_BOOTLOADER_MAGIC) 
    {
        LOG(CRITICAL, "Invalid multiboot magic number (%x)", magic_number);
        printf("Invalid multiboot magic number (%x)\n", magic_number);
        abort();
    }
    if(!((multiboot_info->flags >> 6) & 1)) 
    {
        LOG(CRITICAL, "Invalid memory map");
        printf("Invalid memory map\n");
        abort();
    }

    cpuid_highest_function_parameter = 0xffffffff;
    has_cpuid = true;
    uint32_t ebx, ecx, edx;
    cpuid(0, cpuid_highest_function_parameter, ebx, ecx, edx);

    *(uint32_t*)&manufacturer_id_string[0] = ebx;
    *(uint32_t*)&manufacturer_id_string[4] = edx;
    *(uint32_t*)&manufacturer_id_string[8] = ecx;
    manufacturer_id_string[12] = 0;
    
    current_keyboard_layout = &us_qwerty;

    LOG(INFO, "CPU manufacturer string : \"%s\"", manufacturer_id_string);
    printf("CPU manufacturer string : ");
    tty_set_color(FG_LIGHTRED, BG_BLACK);
    printf("\"%s\"\n", manufacturer_id_string);
    tty_set_color(FG_WHITE, BG_BLACK);

    LOG(INFO, "CPUID highest function parameter : 0x%x", cpuid_highest_function_parameter);
    printf("CPUID highest function parameter : 0x%x\n", cpuid_highest_function_parameter);

    LOG(DEBUG, "FPU test word : 0x%x", fpu_test);
    has_fpu = (fpu_test == 0);
    if (has_fpu)
    {
        LOG(INFO, "FPU found");
        printf("FPU found\n");
    }

    uint32_t cr0 =  get_cr0() | 
                    // ((!has_fpu) << 2) |  // emulate fpu only if there is none // ~ Actually never emulate it
                    (1 << 5) | // you shouldn't try running hos on a i386 anyways...
                    (has_fpu << 1);
    load_cr0(cr0);

    fpu_init_defaults();

    LOG(INFO, "Memory map:");

    printf("Detecting available memory...");

    for (uint32_t i = 0; i < multiboot_info->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + i);
        physical_address_t addr = ((physical_address_t)mmmt->addr_high << 32) | mmmt->addr_low;
        uint64_t len = ((physical_address_t)mmmt->len_high << 32) | mmmt->len_low;
        LOG(INFO, "   Memory block : address : 0x%lx ; length : %lu bytes (type: %s)", addr, len, multiboot_block_type_text[mmmt->type - 1]);
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
            available_memory += len;
    }

    LOG(INFO, "Detected %u bytes of available memory", available_memory); 

    printf(" | Done (%u bytes found)\n", available_memory);

    printf("Loading a GDT...");
    memset(&GDT[0], 0, sizeof(struct gdt_entry));   // NULL Descriptor
    setup_gdt_entry(&GDT[1], 0, 0xfffff, 0b10011011, 0b1100);  // Kernel mode code segment
    setup_gdt_entry(&GDT[2], 0, 0xfffff, 0b10010011, 0b1100);  // Kernel mode data segment
    setup_gdt_entry(&GDT[3], 0, 0xfffff, 0xfa, 0xc);  // User mode code segment
    setup_gdt_entry(&GDT[4], 0, 0xfffff, 0xf2, 0xc);  // User mode data segment
    
    memset(&TSS, 0, sizeof(struct tss_entry));
    // TSS.iopb = sizeof(struct tss_entry);
    TSS.ss0 = KERNEL_DATA_SEGMENT;
    TSS.esp0 = (uint32_t)&stack_top;
    setup_gdt_entry(&GDT[5], (uint32_t)&TSS, sizeof(struct tss_entry) - 1, 0x89, 0);  // TSS
    install_gdt();
    load_tss();
    printf(" | Done\n"); 

    printf("Loading an IDT...");
    install_idt();
    printf(" | Done\n");

    printf("Initializing the PIC...");
    pic_remap(32, 32 + 8);
    printf(" | Done\n");

    printf("Initializing the PIT...");
    pit_channel_0_set_frequency(PIT_FREQUENCY);
    printf(" | Done\n");

    ps2_controller_connected = ps2_device_1_connected = ps2_device_2_connected = false;

    LOG(DEBUG, "Setting up the initrd");

    if (multiboot_info->mods_count != 1)
    {
        LOG(CRITICAL, "Invalid number of modules (%u)", multiboot_info->mods_count);
        printf("Invalid number of modules (%u)\n", multiboot_info->mods_count);
        abort();
    }

    physical_address_t initrd_module_address = multiboot_info->mods_addr;

    initrd_module = (multiboot_module_t*)physical_address_to_virtual(initrd_module_address);    // Conversion is'nt needed because the 1st MB is identity mapped

    LOG(DEBUG, "Initrd module address : 0x%x", initrd_module);
    LOG(DEBUG, "Initrd module start : 0x%x", initrd_module->mod_start);
    LOG(DEBUG, "Initrd module end : 0x%x", initrd_module->mod_end);

    initrd_module->mod_start = physical_address_to_virtual(initrd_module->mod_start);
    initrd_module->mod_end = physical_address_to_virtual(initrd_module->mod_end);

    kernel_end = (virtual_address_t)&_kernel_end;
    kernel_start = (virtual_address_t)&_kernel_start;

    kernel_end = maxint(kernel_end, initrd_module->mod_end);

    initrd_parse();

    kernel_symbols_file = initrd_find_file("./symbols.txt");
    kernel_task_symbols_file = initrd_find_file("./kernel32_symbols.txt");

    LOG(DEBUG, "Setting up memory allocation");

    pfa_detect_usable_memory();

    LOG(DEBUG, "Retrieving CMOS data");
    printf("Retrieving CMOS data...");

    rtc_detect_mode();
    rtc_get_time();

    time_initialized = true;
    
    printf(" | Done\n");

    enable_interrupts(); 
    LOG(DEBUG, "Enabled interrupts"); 

    LOG(DEBUG, "CMOS mode : binary = %u, 24-hour = %u", rtc_binary_mode, rtc_24_hour_mode);
    LOG(INFO, "Time : %u-%u%u-%u%u %u%u:%u%u:%u%u", system_year, system_month / 10, system_month % 10, system_day / 10, system_day % 10, system_hours / 10, system_hours % 10, system_minutes / 10, system_minutes % 10, system_seconds / 10, system_seconds % 10);
    printf("Time : %u-%u%u-%u%u %u%u:%u%u:%u%u\n", system_year, system_month / 10, system_month % 10, system_day / 10, system_day % 10, system_hours / 10, system_hours % 10, system_minutes / 10, system_minutes % 10, system_seconds / 10, system_seconds % 10);

    LOG(DEBUG, "Scanning PCI buses...");
    printf("Scanning PCI buses...");

    pci_scan_buses();
    
    putchar('\n');

    LOG(INFO, "Unix time : %u", ktime(NULL));

    LOG(INFO, "Detecting ACPI tables and ebda");

    bios_get_ebda_pointer();
    acpi_find_tables();

    LOG(INFO, "Detecting PS/2 devices");
    printf("Detecting PS/2 devices\n");

    ps2_device_1_interrupt = ps2_device_2_interrupt = false;

    ps2_flush_buffer();

    ksleep(10);

    ps2_controller_init();
    ps2_detect_keyboards();

    ps2_init_keyboards();

    ksleep(10);
    
    ps2_enable_interrupts();
    
    if (ps2_device_1_connected)
    {
        LOG(INFO, "PS/2 device 1 connected");
        printf("PS/2 device 1 connected\n");
    }
    if (ps2_device_2_connected)
    {
        LOG(INFO, "PS/2 device 2 connected");
        printf("PS/2 device 2 connected\n");
    }
    if (!(ps2_device_1_connected || ps2_device_2_connected))
    {
        LOG(INFO, "No PS/2 devices detected");
        printf("No PS/2 devices detected\n");
    }

    putchar('\n');

    // ~ Debugging
        // // ^ To test stack tracing
        // cause_int_0();

        // physical_address_t addresses[10000000 / 4096] = {0};
        // for(uint32_t i = 0; i < 10000000 / 4096; i++)
        // {
        //     addresses[i] = pfa_allocate_physical_page();
        //     volatile uint8_t a = read_physical_address_1b(addresses[i]);
        // }
        // for(uint32_t i = 0; i < 10000000 / 4096; i++)
        //     pfa_free_physical_page(addresses[10000000 / 4096 - i - 1]);

        // LOG(DEBUG, "0x%x", offsetof(struct local_apic_registers, divide_configuration_register));

    printf("VGA color codes:\n");
    for (uint8_t i = 0; i < 16; i++)
    {
        tty_set_color(FG_BLACK, i << 4);
        putchar(' ');
        putchar(' ');
    }

    tty_set_color(FG_WHITE, BG_BLACK);
    putchar('\n');

    tty_set_color(FG_LIGHTCYAN, BG_BLACK);
    puts(text_logo);
    tty_set_color(FG_WHITE, BG_BLACK);
    putchar('\n');

    LOG(DEBUG, "Initializing multitasking");
    LOG(DEBUG, "memory allocated to TCBs : %u bytes", sizeof(tasks));

    if (1000 % TASK_SWITCH_DELAY != 0) abort(); // ! Task switch delay does not divide a second evenly

    // ~ No need to call it another time
    // fpu_init();

    LOG(DEBUG, "eflags : 0x%x", get_eflags());

    multitasking_init();

    // multitasking_add_task_from_function("kernel32", kmain);
    // multitasking_add_task_from_initrd("kernel32", "./kmain.elf");
    multitasking_add_task_from_initrd("kernel32", "./kernel32.elf");

    multitasking_start();

    while (true);

    halt();
}