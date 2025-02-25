section .text

global _start
_start:
    mov eax, 'A'
.loop:
    int 0xff
    jmp .loop