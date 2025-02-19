section .text

_start:
;    jmp $
    mov eax, 'A'
.loop:
    int 0xff
    jmp .loop