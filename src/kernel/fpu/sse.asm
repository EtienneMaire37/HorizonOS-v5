section .text
bits 64

global get_xsave_area_size
get_xsave_area_size:
    push rcx
    push rdx
    push rbx

    mov eax, 0Dh
    mov ecx, 1
    cpuid
    mov eax, ebx

    pop rbx
    pop rdx
    pop rcx
    ret

global get_supported_xcr0
get_supported_xcr0:
    push rcx
    push rdx
    push rbx
    
    mov eax, 0Dh
    mov ecx, 0
    cpuid
    and rax, 0xffffffff
    shl rdx, 32
    or rax, rdx

    pop rbx
    pop rdx
    pop rcx
    ret