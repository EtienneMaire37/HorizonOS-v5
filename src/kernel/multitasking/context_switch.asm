section .text
bits 32

extern task_esp_offset
extern task_esp0_offset
extern task_cr3_offset

; void __attribute__((cdecl)) context_switch(task_t* old_tcb, task_t* next_tcb)
global context_switch
context_switch:
    ; ret
    push ebx
    push esi
    push edi
    push ebp

    mov ebx, [task_esp_offset]
    mov edi, [esp + (4 + 1) * 4]        ; $edi = (uint32_t)old_tcb
    mov [edi + ebx], esp                ; edi->esp = $esp

    mov esi, [esp + (4 + 2) * 4]        ; $esi = (uint32_t)next_tcb

    mov esp, [esi + ebx]                ; $esp = esi->esp

    mov ebx, [task_cr3_offset]

    mov ecx, cr3
    mov [edi + ebx], ecx
    mov eax, [esi + ebx]

    mov cr3, eax

    cmp eax, ecx
    je .end
    mov cr3, eax
.end:
    pop ebp
    pop edi
    pop esi
    pop ebx

    ret