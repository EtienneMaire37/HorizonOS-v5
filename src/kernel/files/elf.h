#pragma once

typedef uint16_t    elf32_half_t;
typedef uint32_t    elf32_word_t;
typedef uint32_t    elf32_addr_t;
typedef uint32_t    elf32_off_t;
typedef int32_t     elf32_sword_t;

#define ELF_NIDENT 16

#define ELF_CLASS_32    1
#define ELF_CLASS_64    2

#define ELF_DATA_LITTLE_ENDIAN  1
#define ELF_DATA_BIG_ENDIAN     2

#define ELF_OSABI_SYSV  0
#define ELF_INSTRUCTION_SET_x86 3

#define ELF_TYPE_RELOCATABLE    1
#define ELF_TYPE_EXECUTABLE     2
#define ELF_TYPE_SHARED         3
#define ELF_TYPE_CORE           4

struct elf32_header
{
    uint8_t         magic[4];
    uint8_t         architecture;
    uint8_t         byte_order;
    uint8_t         version;
    uint8_t         osabi;
    uint8_t         abiversion;
    uint8_t         pad[7];
    
    elf32_half_t    type;
    elf32_half_t    machine;
    elf32_word_t    version;
    elf32_addr_t    entry;
    elf32_off_t     phoff;
    elf32_off_t     shoff;
    elf32_word_t    flags;
    elf32_half_t    ehsize;
    elf32_half_t    phentsize;
    elf32_half_t    phnum;
    elf32_half_t    shentsize;
    elf32_half_t    shnum;
    elf32_half_t    shstrndx;
};