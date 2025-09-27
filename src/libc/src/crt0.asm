bits 32
section .text

global kernel_data
kernel_data: dd 0

extern _main
global _start
_start:
    mov [kernel_data], esp
    mov ebp, esp
    call .start ; stack frame
    jmp .halt
.start:
    push ebp
    mov ebp, esp

    call _main

    pop ebp
    mov esp, ebp
.halt:
    hlt
    jmp .halt