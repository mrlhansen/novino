ORG 0x1000

[BITS 16]
    mov ax, 0
    mov ss, ax
    mov ds, ax

    mov sp, 0x1000              ; Set stack top
    lgdt [gdt32_ptr]

    mov eax, cr0
    bts eax, 0                  ; Set CR0.PE (bit 0) to enable protected mode
    mov cr0, eax

    jmp 0x08:protectedmode      ; 0x08 = gdt.code

[BITS 32]
protectedmode:
    mov ax, 0x10                ; 0x10 = gdt.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x1000             ; Set stack top
    lgdt [gdt64_ptr]

    mov eax, cr4
    bts eax, 5                  ; Set CR4.PAE to enable physical address extension
    mov cr4, eax

    mov eax, 0x50000            ; The address of the initial PML4 created by the loader
    mov cr3, eax

    mov ecx, 0xC0000080         ; Set MSR address for EFER
    rdmsr
    bts eax, 0                  ; Set syscall extension (SCE) bit
    bts eax, 8                  ; Set long mode (LME) bit
    bts eax, 11                 ; Set no execute (NX) bit
    wrmsr

    mov eax, cr0
    bts eax, 16                 ; Set CR0.WP to enable write-protect
    bts eax, 31                 ; Set CR0.PG to enable paging
    mov cr0, eax

    jmp 0x08:longmode           ; 0x08 = gdt.code

[BITS 64]
longmode:
    add qword [stack], 0x200    ; Increment stack address
    mov rsp, qword [stack]      ; Set stack top
    jmp qword [kernel_entry]    ; The address of the entry point
    jmp $

; Global Descriptor Tables
gdt32_ptr:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start

gdt32_start:
    .null:
        dq 0x0000000000000000
    .code:
        dq 0x00CF9F000000FFFF
    .data:
        dq 0x00CF93000000FFFF
gdt32_end:

gdt64_ptr:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

gdt64_start:
    .null:
        dq 0x0000000000000000
    .code:
        dq 0x00209A0000000000
    .data:
        dq 0x0000920000000000
gdt64_end:

; Location of AP stacks
stack:
    dq 0xFFFFFFFF80010000

; Location of kernel entry
kernel_entry:
