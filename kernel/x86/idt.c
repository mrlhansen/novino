#include <kernel/x86/idt.h>

static idtr_t idt_ptr;
static idt_entry_t idt_entries[256];

void idt_clear_gate(int num)
{
    idt_entries[num].offset_low = 0;
    idt_entries[num].offset_mid = 0;
    idt_entries[num].offset_high = 0;
    idt_entries[num].flags = 0;
    idt_entries[num].selector = 0;
}

void idt_set_gate(int num, void *ptr)
{
    uint64_t offset = (uint64_t)ptr;
    idt_entries[num].offset_low  = (offset & 0xFFFF);
    idt_entries[num].offset_mid  = ((offset >> 16) & 0xFFFF);
    idt_entries[num].offset_high = ((offset >> 32) & 0xFFFFFFFF);
    idt_entries[num].flags = 0x8E; // 10001110: Present = 1, DPL = 0 (Ring 0), Type = 14 (64-bit Interrupt Gate)
    idt_entries[num].selector = 0x08; // Kernel CS
    idt_entries[num].ist = 0;
    idt_entries[num].reserved = 0;
}

void idt_load()
{
    asm volatile("lidt %0" :: "m"(idt_ptr));
}

void idt_init()
{
    idt_ptr.limit = sizeof(idt_entries) - 1;
    idt_ptr.base = (uint64_t)&idt_entries;
    idt_load();
}
