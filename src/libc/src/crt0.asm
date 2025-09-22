bits 32
section .text

extern _main
global _start
_start:
    mov ebp, esp
    call .start ; stack frame
    jmp .halt
.start:
    push ebp
    mov ebp, esp

    call _main
    ; call abc

    pop ebp
    mov esp, ebp
.halt:
    hlt
    jmp .halt

; abc:
;     push ebp
;     mov ebp, esp

;     call def

;     pop ebp
;     mov esp, ebp
;     ret

; def:
;     push ebp
;     mov ebp, esp

;     push 0x0abcdef0
;     push 0x98765432

;     xor ecx, ecx
;     div ecx

;     add esp, 8

;     pop ebp
;     mov esp, ebp
;     ret