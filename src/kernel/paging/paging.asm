bits 64
section .text

global enable_paging
enable_paging:
    mov rax, cr0
    or rax, (1 << 31)
    mov cr0, rax
    ret