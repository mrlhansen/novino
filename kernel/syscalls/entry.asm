[BITS 64]
global syscall_address
extern syscall_table
extern syscall_count

[SECTION .text]
syscall_entry:
    cli
    cmp rax, qword [syscall_count]
    jge .ret

    swapgs
    mov [gs:8], rsp       ; store user rsp
    mov rsp, [gs:0]       ; load kernel rsp
    push qword [gs:8]
    swapgs
    sti

    push rdi              ; rbp, rbx, r12-r15 are callee save registers
    push rsi
    push rdx
    push rcx              ; user rip
    push r8
    push r9
    push r10
    push r11              ; user rflags

    mov rcx, r10
    call qword [syscall_table + 8*rax]

    pop r11               ; user rflags
    pop r10
    pop r9
    pop r8
    pop rcx               ; user rip
    pop rdx
    pop rsi
    pop rdi

    cli
    pop rsp               ; load user rsp
.ret:
    o64 sysret

[SECTION .rodata]
syscall_address:
    dq syscall_entry
