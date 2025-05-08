bits 32
section .text

extern _main
global _start
_start:
    push 0  ; stack frame
    call _main
.halt:
    hlt
    jmp .halt