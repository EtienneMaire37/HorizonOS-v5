#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "multiboot.h"
multiboot_info_t* multiboot_info;

#define KB 1024
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

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

#include "klibc/arithmetic.c"

#include "IO/io.h"
#include "PS2/ps2.h"
#include "debug/out.h"

#include "IO/textio.h"
#include "klibc/stdio.h"
// #include "klibc/reset.h"
#include "klibc/string.h"
#include "klibc/stdlib.h"
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

#include "memalloc/page_frame_allocator.c"
#include "klibc/string.c"
#include "IO/textio.c"
#include "klibc/stdio.c"
#include "klibc/stdlib.c"
#include "GDT/gdt.c"
#include "paging/paging.c"
#include "PIT/pit.c"
#include "IDT/idt.c"
#include "IDT/int.c"
#include "IDT/pic.c"
#include "multitasking/task.c"

// ---------------------------------------------------------------

struct page_table_entry page_table_0[1024] __attribute__((aligned(4096)));
struct page_table_entry page_table_768_1023[256 * 1024] __attribute__((aligned(4096)));

// ---------------------------------------------------------------

physical_address_t virtual_address_to_physical(virtual_address_t address)
{
    if (address < 0x100000) return (physical_address_t)address;
    if (address == 0xffffffff) return virtual_address_to_physical((virtual_address_t)(uint32_t)&page_directory);   // Recursive paging
    if (address >= 0xc0000000) return address - 0xc0000000;
    LOG(CRITICAL, "Invalid virtual address 0x%x", address);
    kabort();
    return 0;
}

virtual_address_t physical_address_to_virtual(physical_address_t address)
{
    if (address < 0x100000) return (virtual_address_t)address;
    if (address < 0xc0000000) return address + 0xc0000000;
    LOG(CRITICAL, "Unmapped physical address 0x%x", address);
    kabort();
    return 0;
}

void halt()
{
    LOG(WARNING, "Kernel halted");
    _halt();
}

void kernel(multiboot_info_t* _multiboot_info, uint32_t magic_number)
{
    multiboot_info = _multiboot_info;
    tty_cursor = 0;
    _kstdin.stream = STDIN_STREAM;
    _kstdout.stream = STDOUT_STREAM;
    _kstderr.stream = STDERR_STREAM;
    _klog.stream = LOG_STREAM;

    kernel_size = &_kernel_end - &_kernel_start;

    tty_clear_screen(' ');
    tty_reset_cursor();

    LOG(INFO, "Kernel loaded at address 0x%x - 0x%x (%u bytes long)", &_kernel_start, &kernel_end, kernel_size); 

    // halt();

    uint32_t max_kernel_size = (uint32_t)(-(uint32_t)&_kernel_start);
    if (kernel_size >= max_kernel_size)
    {
        LOG(CRITICAL, "Kernel is too big (max %uB)", max_kernel_size); 
        kprintf("Kernel is too big (max %uB)\n", max_kernel_size);
        kabort();
    }

    kprintf("Detecting available memory...");

    if(magic_number != MULTIBOOT_BOOTLOADER_MAGIC) 
    {
        LOG(CRITICAL, "Invalid multiboot magic number (%x)", magic_number);
        kprintf("Invalid multiboot magic number (%x)\n", magic_number);
        kabort();
    }
    if(!((multiboot_info->flags >> 6) & 1)) 
    {
        LOG(CRITICAL, "Invalid memory map");
        kprintf("Invalid memory map\n");
        kabort();
    }

    LOG(INFO, "Memory map:");

    for (uint32_t i = 0; i < multiboot_info->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + i);
        physical_address_t addr = ((physical_address_t)mmmt->addr_high << 32) | mmmt->addr_low;
        uint64_t len = ((physical_address_t)mmmt->len_high << 32) | mmmt->len_low;
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) 
        {
            LOG(INFO, "   Memory block : address : 0x%lx ; length : %lu bytes", addr, len);
            available_memory += len;
        }   
    }

    LOG(INFO, "Detected %u bytes of available memory", available_memory); 

    kprintf(" | Done (%u bytes found)\n", available_memory);

    kprintf("Loading a GDT...");
    kmemset(&GDT[0], 0, sizeof(struct gdt_entry));   // NULL Descriptor
    setup_gdt_entry(&GDT[1], 0, 0xfffff, 0b10011011, 0b1100);  // Kernel mode code segment
    setup_gdt_entry(&GDT[2], 0, 0xfffff, 0b10010011, 0b1100);  // Kernel mode data segment
    setup_gdt_entry(&GDT[3], 0, 0xfffff, 0xfa, 0xc);  // User mode code segment
    setup_gdt_entry(&GDT[4], 0, 0xfffff, 0xf2, 0xc);  // User mode data segment
    
    kmemset(&TSS, 0, sizeof(struct tss_entry));
    // TSS.iopb = sizeof(struct tss_entry);
    TSS.ss0 = KERNEL_DATA_SEGMENT;
    TSS.esp0 = (uint32_t)&stack_top;
    setup_gdt_entry(&GDT[5], (uint32_t)&TSS, sizeof(struct tss_entry) - 1, 0x89, 0);  // TSS
    install_gdt();
    load_tss();
    kprintf(" | Done\n");

    LOG(DEBUG, "Loaded the GDT"); 

    kprintf("Loading an IDT...");
    install_idt();
    kprintf(" | Done\n");

    LOG(DEBUG, "Loaded the IDT"); 

    kprintf("Initializing the PIC...");
    pic_remap(32, 32 + 8);
    kprintf(" | Done\n");

    LOG(DEBUG, "Initialized the PIC"); 

    kprintf("Initializing the PIT...");
    pit_channel_0_set_frequency(PIT_FREQUENCY);
    kprintf(" | Done\n");

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
    
    for (uint16_t i = 769; i < 1023; i++)
        add_page_table(page_directory, i, virtual_address_to_physical((virtual_address_t)&page_table_768_1023[(i - 768) * 1024]), PAGING_SUPERVISOR_LEVEL, true);  

    add_page_table(page_directory, 1023, virtual_address_to_physical((virtual_address_t)&page_directory), PAGING_SUPERVISOR_LEVEL, true);    // Setup recursive mapping

    reload_page_directory(); 

    LOG(DEBUG, "Done setting up paging"); 

    LOG(DEBUG, "Setting up the initrd");

    if (multiboot_info->mods_count != 1)
    {
        LOG(CRITICAL, "Invalid number of modules (%u)", multiboot_info->mods_count);
        kprintf("Invalid number of modules (%u)\n", multiboot_info->mods_count);
        kabort();
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
    kprintf("Retrieving CMOS data...\n");

    rtc_detect_mode();
    rtc_get_time();

    time_initialized = true;

    LOG(DEBUG, "CMOS mode : binary = %u, 24-hour = %u", rtc_binary_mode, rtc_24_hour_mode);
    LOG(INFO, "Time : %u:%u:%u %u-%u-%u", system_hours, system_minutes, system_seconds, system_day, system_month, system_year);

    LOG(DEBUG, "Setting up multitasking");

    LOG(TRACE, "sizeof(struct task) : %u", sizeof(struct task));

    multitasking_init();

    multasking_add_task_from_initrd("./bin/initrd/taskA.elf", 3);
    multasking_add_task_from_initrd("./bin/initrd/taskB.elf", 3);
    multasking_add_task_from_initrd("./bin/initrd/taskC.elf", 3);

    multitasking_start();

    halt();
}