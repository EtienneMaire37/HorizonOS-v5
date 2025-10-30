bits 64
section .text

global load_gdt
extern _gdtr
; void LoadGDT()
load_gdt:
    lgdt  [_gdtr]
    ; jmp far 0x08:.reload_seg
    push 0x08
    push .reload_seg
    retf
.reload_seg:
    mov   ax, 0x10
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax
    mov   ss, ax
 
    ret

global load_tss
load_tss:
	mov ax, 0x28 
	ltr ax
	ret
