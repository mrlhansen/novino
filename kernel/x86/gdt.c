#include <kernel/x86/gdt.h>

static gdtr_t gdt_ptr;
static gdt_entry_t gdt_entries[128];

void gdt_set_gate(uint8_t num, uint64_t value)
{
    gdt_entries[num].value = value;
}

void gdt_init()
{
    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = (uint64_t)&gdt_entries;

    gdt_set_gate(0, 0x0000000000000000); // 0x00 - Null segment
    gdt_set_gate(1, 0x00209A0000000000); // 0x08 - Kernel code segment. DPL = 0, Type = Execute/Read
    gdt_set_gate(2, 0x0000920000000000); // 0x10 - Kernel data segment. DPL = 0, Type = Read/Write
    gdt_set_gate(3, 0x0000F20000000000); // 0x18 - User data segment. DPL = 3, Type = Read/Write
    gdt_set_gate(4, 0x0020FA0000000000); // 0x20 - User code segment. DPL = 3, Type = Execute/Read

    gdt_load();
}

void gdt_load()
{
    __asm__("lgdt %0" :: "m"(gdt_ptr));
    __asm__("mov $0x0, %ax");
    __asm__("mov %ax, %fs");
    __asm__("mov %ax, %gs");
    __asm__("mov $0x10, %ax");
    __asm__("mov %ax, %ss");
    __asm__("mov %ax, %ds");
    __asm__("mov %ax, %es");
}
