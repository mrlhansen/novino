[BITS 64]
extern kmain
extern _init
global kstub

kstub:
    push rdi
    call _init
    pop rdi
    xor rbp, rbp
    call kmain
    cli
    hlt
    jmp $
