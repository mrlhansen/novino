[BITS 64]
extern isr_handler
extern irq_handler
extern schedule_handler

[SECTION .text]
%macro ISR_NOERRCODE 1
global isr%1:function (isr%1.end - isr%1)
isr%1:
    cli                   ; Disable interrupts
    push 0                ; Push a dummy error code
    push %1               ; Push the interrupt number
    jmp isr_common_stub
.end:
%endmacro

%macro ISR_ERRCODE 1
global isr%1:function (isr%1.end - isr%1)
isr%1:
    cli                   ; Disable interrupts
    push %1               ; Push the interrupt number
    jmp isr_common_stub
.end:
%endmacro

%macro IRQ 1
global irq%1:function (irq%1.end - irq%1)
irq%1:
    push 0                ; Push a dummy error code
    push %1               ; Push the irq number
    jmp irq_common_stub
.end:
%endmacro

%macro save_registers 0
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov ax, 0x10
    mov ds, ax
    mov es, ax
%endmacro

%macro restore_registers 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31

%assign n 0
%rep 192
IRQ n
%assign n n+1
%endrep

global irq_common_stub:function (irq_common_stub.end - irq_common_stub)
irq_common_stub:
    save_registers
    mov rdi, rsp
    call irq_handler
    restore_registers
    add rsp, qword 16
    iretq
.end:

global isr_common_stub:function (isr_common_stub.end - isr_common_stub)
isr_common_stub:
    save_registers
    mov rdi, rsp
    call isr_handler
    restore_registers
    add rsp, qword 16
    iretq
.end:

global isr_spurious:function (isr_spurious.end - isr_spurious)
isr_spurious:
    iretq
.end:

global switch_task:function (switch_task.end - switch_task)
switch_task:
    pushf
    cli

    mov rax, rsp      ; rax, rcx, rdx are caller save
    mov rcx, ss
    mov rdx, cs

    push rcx          ; ss
    push rax          ; rsp
    pushf             ; rflags
    push rdx          ; cs
    push .next        ; rip

    jmp isr_schedule
.next:
    popf
    ret
.end:

extern xsave_support
global isr_schedule:function (isr_schedule.end - isr_schedule)
isr_schedule:
    cli                             ; mask interrupts
    save_registers                  ; push all general purpose registers to stack

    mov rbp, dr3                    ; address of scheduler struct (rbp is callee save)
    mov ebx, dword [xsave_support]  ; value of xsave support (ebx is callee save)
    mov rax, [rbp+0]                ; address of scheduling stack

    mov rcx, [rbp+8]                ; address of extended state context
    cmp ebx, 0                      ; check for xsave support
    je .fxs                         ; jump if not supported
    xsave [rcx]                     ; save context
.fxs:
    fxsave [rcx]                    ; save context (legacy)

    mov rdi, rsp                    ; rdi is first argument for schedule_handler
    mov rsp, rax                    ; switch to scheduling stack
    call schedule_handler           ; call handler
    mov rsp, rax                    ; load new thread stack

    mov rcx, [rbp+8]                ; address of extended state context
    cmp ebx, 0                      ; check for xsave support
    je .fxr                         ; jump if not supported
    xrstor [rcx]                    ; restore context
.fxr:
    fxrstor [rcx]                   ; restore context (legacy)

    restore_registers               ; pop all general purpose registers from the stack
    iretq                           ; return from interrupt
.end:

[SECTION .rodata]
global irq_stubs:data (irq_stubs.end - irq_stubs)
irq_stubs:
%assign n 0
%rep 192
dq irq%[n]
%assign n n+1
%endrep
.end:
