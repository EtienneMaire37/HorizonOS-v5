bits 32

alignment equ 1
meminfo equ  (1 << 1)   
flags equ (alignment | meminfo)
magic equ 0x1badb002
checksum equ -(magic + flags)

section .multiboot.data
dd magic
dd flags
dd checksum

section .multiboot.text
extern kernel
extern page_directory
extern page_table_0
extern page_table_767
extern page_table_768_1023
global _start
_start:
    cli
    mov [magic_number], eax
    mov [multiboot_info], ebx

    fninit

    mov ebp, stack_top
    sub ebp, 0xc0000000
    mov esp, ebp

    ; Identity map first 4MB
    xor ebx, ebx
    mov ecx, 1024
.loop_pt0:
    mov eax, ebx
    shl eax, 12
    or eax, 0b11
    mov [page_table_0 + 4 * ebx - 0xc0000000], eax
    inc ebx
    loop .loop_pt0

    ; Map physical 0 through PDE 767:1021
    mov eax, 0b1011  ; Present + Writable + Write Through
    mov [page_table_767 - 0xc0000000 + 1021 * 4], eax

    ; Map higher half
    xor edi, edi
.loop_tables:
    xor ebx, ebx
.loop_entries:
    mov eax, edi
    shl eax, 22
    mov edx, ebx
    shl edx, 12
    add eax, edx
    or eax, 0b1011

    mov edx, edi
    imul edx, 1024
    add edx, ebx
    shl edx, 2
    add edx, page_table_768_1023 - 0xc0000000
    mov [edx], eax

    inc ebx
    cmp ebx, 1024
    jl .loop_entries

    inc edi
    cmp edi, 256
    jl .loop_tables

    mov eax, page_table_0 - 0xc0000000
    and eax, 0xfffff000
    or eax, 0b1011
    mov [page_directory - 0xc0000000], eax

    ; pde 767 (physical memory access)
    mov eax, page_table_767 - 0xc0000000
    and eax, 0xfffff000
    or eax, 0b1011
    mov [page_directory - 0xc0000000 + 767*4], eax

    ; pde 768-1023 (higher half)
    mov edi, 768
    mov ecx, 256
.loop_pde:
    mov eax, edi
    sub eax, 768
    shl eax, 12
    add eax, page_table_768_1023 - 0xc0000000
    and eax, 0xfffff000
    or eax, 0b1011
    mov edx, page_directory - 0xc0000000
    mov [edx + edi*4], eax
    inc edi
    loop .loop_pde

    ; Recursive paging
    mov eax, page_directory - 0xc0000000
    and eax, 0xfffff000
    or eax, 0b1011
    mov [page_directory - 0xc0000000 + 4092], eax

    ; Set up paging
    mov ecx, page_directory - 0xc0000000
    mov cr3, ecx

    mov eax, cr0
    or eax, (1 << 31) ; enable paging
    and eax, ~(1 << 30) ; enable cache
    and eax, ~(1 << 29) ; enable write-through
    and eax, ~(1 << 2) ; disable fpu emulation
    and eax, ~(1 << 3) ; don't use hardware multitasking (and force fpu access)
    mov cr0, eax

    lea eax, [.higher_half]
    jmp eax
.higher_half:
    mov ebp, stack_top
    mov esp, ebp
    push ebp

    fninit ; initialize the fpu
    fnstsw [fpu_test] ; write the status word

    push dword [magic_number]
    push dword [multiboot_info]
    call kernel
    add esp, 8

    jmp _halt

magic_number: dd 0
multiboot_info: dd 0

global fpu_test
fpu_test: dw 0x1234

section .text
global _halt
_halt:
.loop:
    cli
    hlt
    jmp .loop

section .bss
global stack_top
global stack_bottom
stack_bottom:
    resb 16384
stack_top: