section .text
bits 64

global enable_avx
enable_avx:
    push rbp
    mov rbp, rsp

    push rax
    push rcx
    push rdx

    mov rax, cr4
    or rax, (1 << 18)   ; * OSXSAVE
    mov cr4, rax

    xor rcx, rcx
    xgetbv
    or eax, 0b111 ; * AVX | SSE | X87
    xsetbv

    pop rdx
    pop rcx
    pop rax

    mov rsp, rbp
    pop rbp

    ret