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

uint64_t kernel_size = 0;
uint64_t available_memory = 0;

extern void _halt();
void halt();

#define enable_interrupts()  asm("sti");
#define disable_interrupts() asm("cli");

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

physical_address_t virtual_address_to_physical(virtual_address_t address);
virtual_address_t physical_address_to_virtual(physical_address_t address);

#include "io/io.h"
#include "ps2/ps2.h"
#include "debug/out.h"

#include "io/textio.h"
#include "klibc/stdio.h"
// #include "klibc/reset.h"
#include "klibc/string.h"
#include "klibc/stdlib.h"
#include "gdt/gdt.h"
#include "paging/paging.h"
#include "pit/pit.h"
#include "idt/idt.h"
#include "idt/int.h"
#include "idt/pic.h"
#include "multitasking/task.h"

#include "io/textio.c"
#include "klibc/stdio.c"
#include "klibc/string.c"
#include "klibc/stdlib.c"
#include "gdt/gdt.c"
#include "paging/paging.c"
#include "pit/pit.c"
#include "idt/idt.c"
#include "idt/int.c"
#include "idt/pic.c"
#include "multitasking/task.c"

// ---------------------------------------------------------------

struct page_table_entry page_table_0[1024] __attribute__((aligned(4096)));
struct page_table_entry page_table_768_1023[256 * 1024] __attribute__((aligned(4096)));

// ---------------------------------------------------------------

physical_address_t virtual_address_to_physical(virtual_address_t address)
{
    return address - 0xc0000000;
}

virtual_address_t physical_address_to_virtual(physical_address_t address)
{
    return address + 0xc0000000;
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

    LOG(INFO, "Kernel loaded at address 0x%x - 0x%x (%u bytes long)", &_kernel_start, &_kernel_end, kernel_size); 

    uint32_t max_kernel_size = (uint32_t)(-(uint32_t)&_kernel_start);
    if (kernel_size >= max_kernel_size)
    {
        LOG(CRITICAL, "Kernel is too big (max %uB)", max_kernel_size); 
        kabort();
    }

    kprintf("Detecting available memory...");

    if(magic_number != MULTIBOOT_BOOTLOADER_MAGIC) 
    {
        LOG(CRITICAL, "Invalid multiboot magic number (%x)", magic_number);
        kabort();
    }
    if(!((multiboot_info->flags >> 6) & 1)) 
    {
        LOG(CRITICAL, "Invalid memory map");
        kabort();
    }

    LOG(INFO, "Memory map:");
    for (uint32_t i = 0; i < multiboot_info->mmap_length; i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + i);
        physical_address_t addr = ((physical_address_t)mmmt->addr_high << 8) | mmmt->addr_low;
        uint32_t len = mmmt->len_low;
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) 
        {
            LOG(INFO, "   Memory block : address : 0x%lx ; length : %u", addr, len);
            available_memory += len;
        }   
    }

    LOG(INFO, "Detected %u bytes of usable memory", available_memory); 

    kprintf(" | Done (%u bytes found)\n", available_memory);

    kprintf("Loading a GDT...");
    kmemset(&GDT[0], 0, sizeof(struct gdt_entry));   // NULL Descriptor
    setup_gdt_entry(&GDT[1], 0, 0xfffff, 0b10011011, 0b1100);  // Kernel mode code segment
    setup_gdt_entry(&GDT[2], 0, 0xfffff, 0b10010011, 0b1100);  // Kernel mode data segment
    setup_gdt_entry(&GDT[3], 0, 0xfffff, 0b11111011, 0b1100);  // User mode code segment
    setup_gdt_entry(&GDT[4], 0, 0xfffff, 0b11110011, 0b1100);  // User mode data segment
    
    kmemset(&TSS, 0, sizeof(struct tss_entry));
    TSS.iopb = sizeof(struct tss_entry);
    TSS.ss0 = KERNEL_DATA_SEGMENT;
    TSS.esp0 = (uint32_t)&stack_top;
    setup_gdt_entry(&GDT[5], (uint32_t)&TSS, sizeof(struct tss_entry), 0b10000000 | TSS_TYPE_32BIT_TSS_AVL, 0b0100);  // TSS
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
    pit_channel_0_set_frequency(1000);
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
            uint32_t address = *(uint32_t*)&layout + 0xc0000000;
            set_page(&page_table_768_1023[i * 1024], j, address, PAGING_SUPERVISOR_LEVEL, true);
        }
    }

    for (uint16_t i = 769; i < 1023; i++)
        add_page_table(page_directory, i, virtual_address_to_physical((virtual_address_t)&page_table_768_1023[(i - 768) * 1024]), PAGING_SUPERVISOR_LEVEL, true);  

    add_page_table(page_directory, 1023, virtual_address_to_physical((virtual_address_t)&page_directory), PAGING_SUPERVISOR_LEVEL, true);    // Setup recursive mapping

    reload_page_directory(); 

    LOG(DEBUG, "Done setting up paging"); 

    struct task task_a = task_init((uint32_t)&task_a_main);
    struct task task_b = task_init((uint32_t)&task_b_main);
    task_a.next_task = task_a.previous_task = &task_b;
    task_b.next_task = task_b.previous_task = &task_a;
    current_task = &task_b;

    multitasking_enabled = true;

    while(true);

    halt();
}