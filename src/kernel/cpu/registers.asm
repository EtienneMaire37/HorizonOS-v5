bits 32
section .text

global get_eflags
get_eflags:
    pushfd
    pop eax
    ret

global set_eflags
set_eflags:
    mov eax, [esp + 4]
    push eax
    popfd
    ret