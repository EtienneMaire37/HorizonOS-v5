section .text
bits 64

global get_xsave_area_size
get_xsave_area_size:
    mov eax, 0Dh
    mov ecx, 1
    cpuid
    mov eax, ebx
    ret