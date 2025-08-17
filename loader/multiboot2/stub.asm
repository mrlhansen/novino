[BITS 32]
global start
global kernel_elf_size
global kernel_elf_address

extern kmain   ; Start of the main C function
extern text    ; Start of the .text section
extern bss     ; Start of the .bss section
extern end     ; End of the last loadable section

TAG_TYPE_END               equ 0
TAG_TYPE_CMDLINE           equ 1
TAG_TYPE_BOOT_LOADER_NAME  equ 2
TAG_TYPE_MODULE            equ 3
TAG_TYPE_BASIC_MEMINFO     equ 4
TAG_TYPE_BOOTDEV           equ 5
TAG_TYPE_MMAP              equ 6
TAG_TYPE_VBE               equ 7
TAG_TYPE_FRAMEBUFFER       equ 8
TAG_TYPE_ELF_SECTIONS      equ 9
TAG_TYPE_APM               equ 10
TAG_TYPE_EFI32             equ 11
TAG_TYPE_EFI64             equ 12
TAG_TYPE_SMBIOS            equ 13
TAG_TYPE_ACPI_OLD          equ 14
TAG_TYPE_ACPI_NEW          equ 15
TAG_TYPE_NETWORK           equ 16
TAG_TYPE_EFI_MMAP          equ 17
TAG_TYPE_EFI_BS            equ 18
TAG_TYPE_EFI32_IH          equ 19
TAG_TYPE_EFI64_IH          equ 20
TAG_TYPE_LOAD_BASE_ADDR    equ 21

HEADER_TAG_END                  equ 0
HEADER_TAG_INFORMATION_REQUEST  equ 1
HEADER_TAG_ADDRESS              equ 2
HEADER_TAG_ENTRY_ADDRESS        equ 3
HEADER_TAG_CONSOLE_FLAGS        equ 4
HEADER_TAG_FRAMEBUFFER          equ 5
HEADER_TAG_MODULE_ALIGN         equ 6
HEADER_TAG_EFI_BS               equ 7
HEADER_TAG_ENTRY_ADDRESS_EFI32  equ 8
HEADER_TAG_ENTRY_ADDRESS_EFI64  equ 9
HEADER_TAG_RELOCATABLE          equ 10

HEADER_MAGIC     equ 0xE85250D6
HEADER_ARCH      equ 0
HEADER_LENGTH    equ (multiboot_header_end - multiboot_header_start)
HEADER_CHECKSUM  equ -(HEADER_LENGTH + HEADER_MAGIC + HEADER_ARCH)

SECTION .rodata
ALIGN 8
multiboot_header_start:
    dd HEADER_MAGIC                   ; Multiboot2 magic value
    dd HEADER_ARCH                    ; Architecture (i386, protected mode)
    dd HEADER_LENGTH                  ; Header length including all tags
    dd HEADER_CHECKSUM                ; Header checksum

    dw HEADER_TAG_ADDRESS             ; Address tag
    dw 0                              ; Flags (0 = optional, 1 = required)
    dd 24                             ; Size in bytes
    dd multiboot_header_start         ; Start of multiboot header
    dd text                           ; Start of text section
    dd bss                            ; End of data section
    dd end                            ; End of bss section

    dw HEADER_TAG_ENTRY_ADDRESS       ; Entry address tag
    dw 0                              ; Flags (0 = optional, 1 = required)
    dd 12                             ; Size in bytes
    dd start                          ; Start execution here
    dd 0                              ; Padding (align 8)

    dw HEADER_TAG_FRAMEBUFFER         ; Framebuffer tag
    dw 0                              ; Flags (0 = optional, 1 = required)
    dd 20                             ; Size in bytes
    dd 1920                           ; Width (0 = no preference)
    dd 1080                           ; Height (0 = no preference)
    dd 32                             ; Depth (0 = no preference)
    dd 0                              ; Padding (align 8)

    dw HEADER_TAG_INFORMATION_REQUEST ; Information request tag
    dw 0                              ; Flags (0 = optional, 1 = required)
    dd 40                             ; Size in bytes
    dd TAG_TYPE_CMDLINE               ; Command line options
    dd TAG_TYPE_BOOT_LOADER_NAME      ; Boot loader name
    dd TAG_TYPE_MODULE                ; Information about modules
    dd TAG_TYPE_BASIC_MEMINFO         ; Simple memory size
    dd TAG_TYPE_MMAP                  ; Detailed memory map
    dd TAG_TYPE_FRAMEBUFFER           ; Framebuffer information
    dd TAG_TYPE_ACPI_OLD              ; Copy of RDSPv1
    dd TAG_TYPE_ACPI_NEW              ; Copy of RDSPv2

    dw HEADER_TAG_END                 ; End tag
    dw 0                              ; Flags (0 = optional, 1 = required)
    dd 8                              ; Size in bytes
multiboot_header_end:

SECTION .text
make_pdp_entries:
    add eax, 00000011b           ; Kernel, write, present
.redo:
    mov dword [edi+0], eax
    mov dword [edi+4], 0
    add edi, 8                   ; Next PDP entry
    add eax, 0x1000              ; Location of next PD table
    loop .redo
    ret

make_pd_entries:
    add eax, 10000011b           ; 2 mb pages, kernel, write, present
.redo:
    mov dword [edi+0], eax
    mov dword [edi+4], 0
    add edi, 8                   ; Next PD entry
    add eax, 0x200000            ; Location of next physical memory chunk
    loop .redo
    ret

start:
    cli                          ; Disable interrupts
    mov dword [mb_struct], ebx   ; Save multiboot information

    lgdt [gdt_ptr]               ; Load GDT

    mov eax, cr4
    bts eax, 5                   ; Enable PAE
    mov cr4, eax

    mov edi, 0x50000             ; Address of PML4
    mov ecx, 0x4000              ; Size of paging structure (16384 * 4 bytes = 64 KB)
    xor eax, eax
    rep stosd                    ; Clear memory for all tables

    mov dword [0x50000], 0x51003 ; Store the address of the 0th PDP table in the PML4 table
    mov dword [0x50FF8], 0x56003 ; Store the address of the 511th PDP table in the PML4 table

    mov edi, 0x51000             ; Address of the 0th PDP table, offset 0
    mov eax, 0x52000             ; Address of the new PD tables
    mov ecx, 4                   ; Create 4 PD tables

    call make_pdp_entries        ; Loop until all PDP entries are created

    mov edi, 0x52000             ; Address of the PD tables created above
    mov eax, 0                   ; Physical memory offset
    mov ecx, 2048                ; 2048 * 2MB = 4GB

    call make_pd_entries         ; Loop until all PD entries are created

    mov edi, 0x56FF0             ; Address of the 511th PDP table, offset 510
    mov eax, 0x57000             ; Address of the new PD table
    mov ecx, 2                   ; Create 2 PD tables

    call make_pdp_entries        ; Loop until all PDP entries are created

    mov edi, 0x57000             ; Address of the PD tables created above
    mov eax, 0                   ; Physical memory offset
    mov ecx, 1024                ; 1024 * 2MB = 2GB

    call make_pd_entries         ; Loop until all PD entries are created

    mov eax, 0x50000             ; Set PML4 base
    mov cr3, eax                 ; Load PML4 base

    mov ecx, 0xC0000080          ; Set MSR address for EFER
    rdmsr
    bts eax, 0                   ; Set syscall extension (SCE) bit
    bts eax, 8                   ; Set long mode (LME) bit
    bts eax, 11                  ; Set no execute (NX) bit
    wrmsr

    mov edx, cr0
    bts edx, 16                  ; Enable write-protect
    bts edx, 31                  ; Enable paging
    mov cr0, edx

    jmp 0x08:long_start

[BITS 64]
long_start:
    mov edi, 0x2000              ; Address of kernel stack
    mov ecx, 0x1000              ; Size of stack (4096 * 4 bytes = 16 KB)
    xor eax, eax
    rep stosd                    ; Clear stack

    mov rsp, 0xFFFFFFFF80006000  ; Setup the stack
    mov edi, dword [mb_struct]   ; Push multiboot information

    jmp kmain

    cli
    hlt
    jmp $

SECTION .data
mb_struct:
    dd 0

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt_start:
    .null:
        dq 0x0000000000000000
    .code:
        dq 0x00209A0000000000
    .data:
        dq 0x0000920000000000
gdt_end:

kernel_elf_size:
    dq kernel_elf_end - kernel_elf_start

kernel_elf_address:
    dq kernel_elf_start

kernel_elf_start:
    incbin "../../kernel/kernel.elf"
kernel_elf_end:
