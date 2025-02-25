bits 32
section .text

global load_idt
extern _idtr
; void load_idt()
load_idt:
    lidt [_idtr]
    ret

%macro INT_ERROR_CODE 1
INT_%1:
    push %1

    jmp _interrupt_handler
%endmacro

%macro INT_NO_ERROR_CODE 1
INT_%1:
    push 0    
    push %1

    jmp _interrupt_handler
%endmacro

extern interrupt_handler

; ISRs
INT_NO_ERROR_CODE 0
INT_NO_ERROR_CODE 1
INT_NO_ERROR_CODE 2
INT_NO_ERROR_CODE 3
INT_NO_ERROR_CODE 4
INT_NO_ERROR_CODE 5
INT_NO_ERROR_CODE 6
INT_NO_ERROR_CODE 7
INT_ERROR_CODE    8
INT_NO_ERROR_CODE 9
INT_ERROR_CODE    10
INT_ERROR_CODE    11
INT_ERROR_CODE    12
INT_ERROR_CODE    13
INT_ERROR_CODE    14
INT_NO_ERROR_CODE 15
INT_NO_ERROR_CODE 16
INT_ERROR_CODE    17
INT_NO_ERROR_CODE 18
INT_NO_ERROR_CODE 19
INT_NO_ERROR_CODE 20
INT_NO_ERROR_CODE 21
INT_NO_ERROR_CODE 22
INT_NO_ERROR_CODE 23
INT_NO_ERROR_CODE 24
INT_NO_ERROR_CODE 25
INT_NO_ERROR_CODE 26
INT_NO_ERROR_CODE 27
INT_NO_ERROR_CODE 28
INT_NO_ERROR_CODE 29
INT_ERROR_CODE    30
INT_NO_ERROR_CODE 31

; IRQs
%assign i 32
    %rep    16
        INT_NO_ERROR_CODE i
    %assign i i+1 
    %endrep

; All the other interrupts
%assign i 48
    %rep    (256 - 48)
        INT_NO_ERROR_CODE i
    %assign i i+1 
    %endrep

global interrupt_table
interrupt_table:
    %assign i 0 
    %rep    256
        dd INT_%+i
    %assign i i+1 
    %endrep

_interrupt_handler:
    pusha

    xor eax, eax
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov eax, cr2
    push eax

    mov eax, cr3
    push eax
    
    push esp
    call interrupt_handler
    mov cr3, eax

    pop esp

    add esp, 8  ; skip cr2 and cr3

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; mov ss, ax ; ss is supposedly handled by iret

    popa
    add esp, 8
    iret