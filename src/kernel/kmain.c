// #include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "multiboot.h"
multiboot_info_t* multiboot_info;

#define KB 1024
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

// #include "../libc/include/inttypes.h"
#include "../libc/include/errno.h"
#include "../libc/include/stddef.h"
#include "../libc/include/stdarg.h"
#include "../libc/include/unistd.h"
#include "../libc/include/sys/types.h"

typedef uint64_t physical_address_t;
typedef uint32_t virtual_address_t;

extern uint8_t stack_top;

extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

virtual_address_t kernel_start;
virtual_address_t kernel_end;

uint64_t kernel_size = 0;
uint64_t available_memory = 0;

extern void _halt();
void halt();

typedef struct FILE FILE;

#define klog ((FILE*)3)

#define enable_interrupts()  asm("sti");
#define disable_interrupts() asm("cli");

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define bcd_to_binary(bcd) (((bcd) & 0x0f) + ((bcd) >> 4) * 10)

physical_address_t virtual_address_to_physical(virtual_address_t address);
virtual_address_t physical_address_to_virtual(physical_address_t address);

multiboot_module_t* initrd_module;

#define LOG_LEVEL           DEBUG
// #define NO_LOGS

const char* multiboot_block_type_text[5] = 
{
    "MULTIBOOT_MEMORY_AVAILABLE",
    "MULTIBOOT_MEMORY_RESERVED",
    "MULTIBOOT_MEMORY_ACPI_RECLAIMABLE",
    "MULTIBOOT_MEMORY_NVS",
    "MULTIBOOT_MEMORY_BADRAM"
};

// #include "klibc/arithmetic.c"
#include "../libc/src/arithmetic.c"

#include "IO/io.h"
#include "PS2/ps2.h"
#include "debug/out.h"

// #include "IO/keyboard.h"
#include "PS2/keyboard.h"
#include "ACPI/tables.h"
#include "IO/textio.h"
// #include "klibc/stdio.h"
// #include "klibc/string.h"
// #include "klibc/stdlib.h"
#include "../libc/include/stdio.h"
#include "../libc/include/string.h"
#include "../libc/include/stdlib.h"
#include "../libc/src/stdio.c"
#include "../libc/src/string.c"
#include "../libc/src/stdlib.c"
#include "GDT/gdt.h"
#include "paging/paging.h"
#include "PIT/pit.h"
#include "IDT/idt.h"
#include "IDT/int.h"
#include "IDT/pic.h"
#include "multitasking/task.h"
#include "CMOS/cmos.h"
#include "CMOS/rtc.h"
#include "memalloc/page_frame_allocator.h"
#include "files/ustar.h"
#include "files/elf.h"
#include "initrd/initrd.h"
#include "time/gdn.h"
#include "time/ktime.h"
// #include "klibc/time.h"
#include "../libc/include/time.h"
#include "../libc/src/time.c"

#include "PS2/keyboard.c"
#include "ACPI/tables.c"
#include "memalloc/page_frame_allocator.c"
// #include "klibc/string.c"
#include "IO/textio.c"
// #include "klibc/stdio.c"
// #include "klibc/stdlib.c"
#include "GDT/gdt.c"
#include "paging/paging.c"
#include "PIT/pit.c"
#include "IDT/idt.c"
#include "IDT/int.c"
#include "IDT/pic.c"
#include "multitasking/task.c"
#include "PS2/ps2.c"

#include "../libc/src/kernel_glue.h"
#include "../libc/src/kernel.c"

// ---------------------------------------------------------------

struct page_table_entry page_table_0[1024] __attribute__((aligned(4096)));
struct page_table_entry page_table_767[1024] __attribute__((aligned(4096)));
struct page_table_entry page_table_768_1023[256 * 1024] __attribute__((aligned(4096)));

// ---------------------------------------------------------------

physical_address_t virtual_address_to_physical(virtual_address_t address)
{
    if (address < 0x100000) return (physical_address_t)address;
    if (address == 0xffffffff) return virtual_address_to_physical((virtual_address_t)(uint32_t)&page_directory);   // Recursive paging
    if (address >= 0xc0000000) return address - 0xc0000000;
    LOG(CRITICAL, "Invalid virtual address 0x%x", address);
    abort();
    return 0;
}

virtual_address_t physical_address_to_virtual(physical_address_t address)
{
    if (address < 0x100000) return (virtual_address_t)address;
    if (address < 0x40000000) return address + 0xc0000000;
    LOG(CRITICAL, "Unmapped physical address 0x%x", address);
    abort();
    return 0;
}

void halt()
{
    LOG(WARNING, "Kernel halted");
    _halt();
}

void __attribute__((cdecl)) kernel(multiboot_info_t* _multiboot_info, uint32_t magic_number)
{
    multiboot_info = _multiboot_info;
    tty_cursor = 0;

    current_phys_mem_page = 0;
    kernel_size = &_kernel_end - &_kernel_start;

    current_cr3 = virtual_address_to_physical((virtual_address_t)page_directory);

    tty_clear_screen(' ');
    tty_reset_cursor();

    LOG(INFO, "Kernel loaded at address 0x%x - 0x%x (%u bytes long)", &_kernel_start, &kernel_end, kernel_size); 

    // halt();

    uint32_t max_kernel_size = (uint32_t)(-(uint32_t)&_kernel_start);
    if (kernel_size >= max_kernel_size)
    {
        LOG(CRITICAL, "Kernel is too big (max %uB)", max_kernel_size); 
        printf("Kernel is too big (max %uB)\n", max_kernel_size);
        abort();
    }

    printf("Detecting available memory...");

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

    LOG(INFO, "Memory map:");

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

    LOG(DEBUG, "Loaded the GDT"); 

    printf("Loading an IDT...");
    install_idt();
    printf(" | Done\n");

    LOG(DEBUG, "Loaded the IDT"); 

    printf("Initializing the PIC...");
    pic_remap(32, 32 + 8);
    printf(" | Done\n");

    LOG(DEBUG, "Initialized the PIC"); 

    printf("Initializing the PIT...");
    pit_channel_0_set_frequency(PIT_FREQUENCY);
    printf(" | Done\n");

    ps2_controller_connected = ps2_device_1_connected = ps2_device_2_connected = false;

    LOG(DEBUG, "Initialized the PIT"); 

    enable_interrupts(); 
    LOG(DEBUG, "Enabled interrupts"); 

    LOG(DEBUG, "Setting up paging"); 

    for (uint16_t i = 256; i < 1024; i++)
        remove_page(&page_table_0[0], i);

    for (uint16_t i = 1; i < 256; i++)
    {
        for (uint16_t j = 0; j < 1024; j++)
        {
            struct virtual_address_layout layout;
            layout.page_directory_entry = i + 768;
            layout.page_table_entry = j;
            layout.page_offset = 0;
            uint32_t address = *(uint32_t*)&layout - 0xc0000000;
            set_page(&page_table_768_1023[i * 1024], j, address, PAGING_SUPERVISOR_LEVEL, true);
        }
    }

    for (uint16_t i = 0; i < 1024; i++)
        remove_page(page_table_767, i);

    set_page(page_table_767, 1021, 0, PAGING_SUPERVISOR_LEVEL, true);
    
    for (uint16_t i = 769; i < 1023; i++)
        add_page_table(page_directory, i, virtual_address_to_physical((virtual_address_t)&page_table_768_1023[(i - 768) * 1024]), PAGING_SUPERVISOR_LEVEL, true);  

    add_page_table(page_directory, 767, virtual_address_to_physical((virtual_address_t)page_table_767), PAGING_SUPERVISOR_LEVEL, true);  

    add_page_table(page_directory, 1023, virtual_address_to_physical((virtual_address_t)page_directory), PAGING_SUPERVISOR_LEVEL, true);    // Setup recursive mapping

    reload_page_directory(); // * Caching is disabled 

    LOG(DEBUG, "Done setting up paging"); 

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

    kernel_end = max(kernel_end, initrd_module->mod_end);

    initrd_parse();

    LOG(DEBUG, "Setting up memory allocation");

    pfa_detect_usable_memory();
    pfa_bitmap_init();

    LOG(DEBUG, "Retrieving CMOS data");
    printf("Retrieving CMOS data...");

    rtc_detect_mode();
    rtc_get_time();

    time_initialized = true;
    
    printf(" | Done\n");

    LOG(DEBUG, "CMOS mode : binary = %u, 24-hour = %u", rtc_binary_mode, rtc_24_hour_mode);
    LOG(INFO, "Time : %u:%u:%u %u-%u-%u", system_hours, system_minutes, system_seconds, system_day, system_month, system_year);
    printf("Time : %u:%u:%u %u-%u-%u\n", system_hours, system_minutes, system_seconds, system_day, system_month, system_year);

    LOG(DEBUG, "Setting up multitasking");

    LOG(TRACE, "sizeof(struct task) : %u", sizeof(struct task));

    putchar('\n');

    LOG(INFO, "Unix time : %u", ktime(NULL));

    LOG(INFO, "Detecting ACPI tables and EBDA");

    bios_get_ebda_pointer();
    acpi_find_tables();

    LOG(INFO, "Detecting PS/2 devices");
    printf("Detecting PS/2 devices\n");

    ps2_device_1_interrupt = ps2_device_2_interrupt = false;

    ps2_flush_buffer();

    ksleep(100);

    ps2_controller_init();
    // ps2_detect_devices();
    ps2_detect_keyboards();

    ps2_init_keyboards();

    ps2_controller_connected = true;
    ps2_device_1_connected = true;
    ps2_device_1_type = PS2_DEVICE_KEYBOARD;

    ksleep(100);
    
    ps2_enable_interrupts();

    // ps2_flush_buffer();
    
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

    // for (uint16_t i = 0; i < 512;)
    // {
    //     uint16_t _i = read_physical_address(virtual_address_to_physical((virtual_address_t)&i)) + 0x100 * read_physical_address(virtual_address_to_physical((virtual_address_t)&i + 1));
    //     LOG(DEBUG, "Address 0x%lx = 0x%x", virtual_address_to_physical((virtual_address_t)&i), _i);
    //     _i++;
    //     write_physical_address(virtual_address_to_physical((virtual_address_t)&i), _i);
    //     write_physical_address(virtual_address_to_physical((virtual_address_t)&i + 1), _i >> 8);
    // }

    // while(true);

    multitasking_init();

    multasking_add_task_from_initrd("./bin/initrd/kernel32.elf", 0, true);

    multitasking_start();

    halt();
}