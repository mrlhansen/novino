[BITS 64]
global ap_boot_code
global ap_boot_size
extern smp_ap_entry

[SECTION .rodata]
ap_boot_code:
    dq ap_boot_start

ap_boot_size:
    dw ap_boot_end - ap_boot_start

ap_boot_start:
    incbin "apboot.bin"
    dq smp_ap_entry
ap_boot_end:
