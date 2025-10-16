section .text
bits 32

global avx_enable
avx_enable:
    push ebp
    mov ebp, esp

    push eax
    push ecx
    push edx

    mov eax, cr4
    or eax, (1 << 18)   ; * OSXSAVE
    mov cr4, eax

    xor ecx, ecx
    xgetbv
    or eax, 0b111 ; * AVX | SSE | X87
    xsetbv

    pop edx
    pop ecx
    pop eax

    mov esp, ebp
    pop ebp

    ret