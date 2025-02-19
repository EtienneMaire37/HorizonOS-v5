section .text

_start:
;    jmp $
    mov eax, 'B'
.loop:
    int 0xff
    jmp .loop