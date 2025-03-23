#pragma once

#define PAGING_SUPERVISOR_LEVEL     0
#define PAGING_USER_LEVEL           1

struct page_directory_entry_4kb
{
    uint8_t present : 1;
    uint8_t read_write : 1;     // 0: Read only ; 1: Read-write
    uint8_t user_supervisor : 1; // 0: Supervisor ; 1: User
    uint8_t write_through : 1;
    uint8_t cache_disable : 1;
    uint8_t accessed : 1;
    uint8_t avl_1 : 1;
    uint8_t page_size : 1; // 0 for 4 KB entry
    uint8_t avl_0 : 4;
    uint32_t address : 20;  // High bits
} __attribute__((packed));

struct page_table_entry
{
    uint8_t present : 1;
    uint8_t read_write : 1;   
    uint8_t user_supervisor : 1;  
    uint8_t write_through : 1;
    uint8_t cache_disable : 1;
    uint8_t accessed : 1;
    uint8_t dirty : 1;
    uint8_t page_attribute_table : 1;
    uint8_t global : 1; 
    uint8_t avl_0 : 3;
    uint32_t address : 20;  // High bits
} __attribute__((packed));

struct virtual_address_layout
{
    uint16_t page_offset : 12;
    uint16_t page_table_entry : 10;
    uint16_t page_directory_entry : 10;
} __attribute__((packed));

// struct PageFault_ErrorCode
// {
//     uint8_t present : 1;    // 0: Non-present page ; 1: Page-Protection violatiob
//     uint8_t write : 1;      // 0: Read Access ; 1: Write Access
//     uint8_t user : 1;       // 0: CPL = 0-2 ; 1: CPL = 3
//     uint8_t reserved_0 : 1;
//     uint8_t instruction_fetch : 1;  // 0: PF caused by data access ; 1: PF caused by instruction fetch 
//     uint8_t protection_key : 1;     // 1: PF caused by protection-key violation
//     uint8_t shadow_stack : 1;       // 1: PF caused by shadow stack access
//     uint8_t reserved_1 : 8;
//     uint8_t SGX : 1;                // 1: PF caused by SGX violation
//     uint8_t reserved_2 : 16;
// };

struct page_directory_entry_4kb page_directory[1024] __attribute__((aligned(4096)));

#define PHYS_MEM_PAGE_BOTTOM   (0xc0000000 - 3 * 0x1000)
#define PHYS_MEM_PAGE_TOP      (PHYS_MEM_PAGE_BOTTOM + 0x1000)
uint32_t current_phys_mem_page;

void init_page_directory(struct page_directory_entry_4kb* pd);
void init_page_table(struct page_table_entry* pt);
void add_page_table(struct page_directory_entry_4kb* pd, uint16_t index, physical_address_t pt_address, uint8_t user_supervisor, uint8_t read_write);
void remove_page_table(struct page_directory_entry_4kb* pd, uint16_t index);
void remove_page(struct page_table_entry* pt, uint16_t index);
void set_page(struct page_table_entry* pt, uint16_t index, physical_address_t address, uint8_t user_supervisor, uint8_t read_write);

void physical_init_page_directory(physical_address_t pd);
void physical_init_page_table(physical_address_t pt);
void physical_add_page_table(physical_address_t pd, uint16_t index, physical_address_t pt_address, uint8_t user_supervisor, uint8_t read_write);
void physical_remove_page_table(physical_address_t pd, uint16_t index);
void physical_remove_page(physical_address_t pt, uint16_t index);
void physical_set_page(physical_address_t pt, uint16_t index, physical_address_t address, uint8_t user_supervisor, uint8_t read_write);

uint8_t read_physical_address_1b(physical_address_t address);
uint16_t read_physical_address_2b(physical_address_t address);
uint32_t read_physical_address_4b(physical_address_t address);
void write_physical_address_1b(physical_address_t address, uint8_t value);
void write_physical_address_2b(physical_address_t address, uint16_t value);
void write_physical_address_4b(physical_address_t address, uint32_t value);

extern void reload_page_directory();
extern void enable_paging(); 