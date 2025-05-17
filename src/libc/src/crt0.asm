bits 32
section .text

extern _main
global _start
_start:
    push dword 0  ; stack frame
    call _main
.halt:
    hlt
    jmp .halt