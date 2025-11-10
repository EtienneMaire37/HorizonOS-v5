bits 64
section .text

default rel

global kernel_data
kernel_data: dq 0

extern _main
global _start
_start:
    pop qword [kernel_data]
    mov rbp, 0
    call .start ; stack frame
    jmp .halt
.start:
    push rbp
    mov rbp, rsp

    call _main

    pop rbp
    mov rsp, rbp

.halt:
    jmp $

extern main
extern exit

global call_main_exit
call_main_exit:
    ; ? arguments are passed as registers, no need to set them up
    call main

    mov rdi, rax ; * main return value
    call exit

    ret
