bits 32
section .text

extern page_directory
global reload_page_directory
reload_page_directory:
    mov eax, page_directory
    sub eax, 0xc0000000
    mov cr3, eax

    ret 

global enable_paging
enable_paging:
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax
    ret