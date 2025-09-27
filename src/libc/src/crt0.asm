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

extern main
extern exit

global call_main_exit
call_main_exit:
    push ebp
    mov ebp, esp

    push dword [esp + 12]   ; argv
    push dword [esp + 12]   ; argc
    call main
    add esp, 8

    push eax        ; main return code
    call exit
    add esp, 4

    pop ebp
    mov esp, ebp
    ret