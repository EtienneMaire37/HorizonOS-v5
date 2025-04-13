ENTRY(_start)

INPUT
(
	"bin/kernelentry.o"
 	"bin/kmain.o"
	"bin/gdt.o"
	"bin/idt.o" 
	"bin/paging.o"
)

OUTPUT_FORMAT(elf32-i386)
OUTPUT("bin/kernel.elf")
 
SECTIONS
{
    . = 0x100000;

    .multiboot : ALIGN(0x1000)
    {
        *(.multiboot.data)
        *(.multiboot.text)
    }
    
    . += 0xc0000000;
 
    .text ALIGN(0x1000) : AT(ADDR(.text) - 0xc0000000)
    {
        _kernel_start = .;
        *(.text .text.*)
    }
 
    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xc0000000)
    {
        *(.rodata .rodata.*)
    }

    .data ALIGN(0x1000) : AT(ADDR(.data) - 0xc0000000)
    {
        *(.data .data.*)
    }

    .bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xc0000000)
    {
        *(COMMON)
        *(.bss .bss.*)
        . = ALIGN(0x1000);
        _kernel_end = .;
    }
    
    /DISCARD/ : { *(.comment) *(.note) }
}