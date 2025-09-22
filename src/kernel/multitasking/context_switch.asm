section .text
bits 32

extern task_esp_offset
extern task_cr3_offset

; void __attribute__((cdecl)) context_switch(thread_t* old_tcb, thread_t* next_tcb, uint32_t ds)
global context_switch
context_switch:
    ; * eax, ecx and edx are caller-saved * ;

    push ebx
    push esi
    push edi
    push ebp

    mov ebx, [task_esp_offset]
    mov edi, [esp + (4 + 1) * 4]        ; $edi = (uint32_t)old_tcb
    mov [edi + ebx], esp                ; edi->esp = $esp

    mov esi, [esp + (4 + 2) * 4]        ; $esi = (uint32_t)next_tcb

    mov edx, [esp + (4 + 3) * 4]        ; $edx = ds

    mov esp, [esi + ebx]                ; $esp = esi->esp

    mov ebx, [task_cr3_offset]

    mov ecx, cr3
    mov [edi + ebx], ecx
    mov eax, [esi + ebx]

    cmp eax, ecx
    je .end
    mov cr3, eax

.end:
    pop ebp
    pop edi
    pop esi
    pop ebx

    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx

    ret