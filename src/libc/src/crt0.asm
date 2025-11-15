bits 64
section .text

default rel

extern _main
global _start
_start:
    pop qword [kernel_data]
    mov rbp, 0
    call .start ; stack frame
    jmp .halt
.start:
    push 0
    mov rbp, 0

    and rsp, 0xfffffffffffffff0

    call _main

.halt:
    jmp $

extern main
extern exit

global call_main_exit
call_main_exit:
    mov rbp, 0
    push rbp

    and rsp, 0xfffffffffffffff0

    ; ? arguments are passed as registers, no need to set them up
    call main

    mov rdi, rax ; * main return value
    call exit

    jmp .halt

section .data

global kernel_data
kernel_data: dq 0