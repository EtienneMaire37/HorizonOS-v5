section .text

global _start
_start:
    ; jmp $
    mov eax, 'C'
.loop:
    int 0xff
    jmp .loop