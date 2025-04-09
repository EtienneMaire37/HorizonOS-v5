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
    mov eax, 0b11  ; Present + Writable
    mov [page_table_767 - 0xc0000000 + 1021*4], eax

    ; Map 1GB higher half
    xor edi, edi
.loop_tables:
    xor ebx, ebx
.loop_entries:
    mov eax, edi
    shl eax, 22
    mov edx, ebx
    shl edx, 12
    add eax, edx
    or eax, 0b11

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

    ; Configure page directory
    mov eax, page_table_0 - 0xc0000000
    and eax, 0xfffff000
    or eax, 0b11
    mov [page_directory - 0xc0000000], eax

    ; PDE 767 (physical memory access)
    mov eax, page_table_767 - 0xc0000000
    and eax, 0xfffff000
    or eax, 0b11
    mov [page_directory - 0xc0000000 + 767*4], eax

    ; PDE 768-1023 (higher half)
    mov edi, 768
    mov ecx, 256
.loop_pde:
    mov eax, edi
    sub eax, 768
    shl eax, 12
    add eax, page_table_768_1023 - 0xc0000000
    and eax, 0xfffff000
    or eax, 0b11
    mov edx, page_directory - 0xc0000000
    mov [edx + edi*4], eax
    inc edi
    loop .loop_pde

    ; Recursive mapping
    mov eax, page_directory - 0xc0000000
    and eax, 0xfffff000
    or eax, 0b11
    mov [page_directory - 0xc0000000 + 4092], eax

    ; Enable paging
    mov ecx, page_directory - 0xc0000000
    mov cr3, ecx
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    lea eax, [.higher_half]
    jmp eax
.higher_half:
    mov ebp, stack_top
    mov esp, ebp

    push dword [magic_number]
    push dword [multiboot_info]
    call kernel
    add esp, 8

    jmp _halt

magic_number: dd 0
multiboot_info: dd 0

section .text
global _halt
_halt:
.loop:
    cli
    hlt
    jmp .loop

section .data
global stack_top
stack_bottom:
    resb 16384
stack_top: