ENTRY(_start)

OUTPUT_FORMAT(elf32-i386)

PHDRS 
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
    bss PT_LOAD FLAGS(6);
}

SECTIONS 
{
    . = 0x100000;

    .text ALIGN(0x1000) : 
    {
        *(.text .text.*)
    } :text

    .rodata ALIGN(0x1000) : 
    {
        *(.rodata .rodata.*)
    } :text

    .data ALIGN(0x1000) : 
    {
        *(.data .data.*)
    } :data

    .bss ALIGN(0x1000) : 
    {
        *(COMMON)
        *(.bss .bss.*)
        . = ALIGN(0x1000);
        _break_point = .;
    } :bss

    /DISCARD/ : { *(.comment) *(.note) }
}