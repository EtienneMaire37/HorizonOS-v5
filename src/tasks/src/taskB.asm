section .text

_start:
    mov eax, 'B'
.loop:
    int 0xff
    jmp .loop