[BITS 64]

; Segment descriptors
; 0x1B = (DS | DPL) = (0x18 | 3)
; 0x23 = (CS | DPL) = (0x20 | 3)

global switch_to_user_mode:function (switch_to_user_mode.end - switch_to_user_mode)
switch_to_user_mode:
    cli
    mov ax, 0x1B
    mov ds, ax
    mov es, ax
    push 0x1B        ; ss
    push rsi         ; rsp
    push 0x202       ; rflags (enable interrupts)
    push 0x23        ; cs
    push rdi         ; rip
    mov rdi, rdx     ; argv
    mov rsi, rcx     ; envp
    iretq
.end:
