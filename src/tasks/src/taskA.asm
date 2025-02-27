section .text

global _start
_start:
    ; cli
    mov eax, 'A'
.loop:
    int 0xff
    jmp .loop