#include <kernel/x86/tss.h>
#include <kernel/x86/gdt.h>
#include <string.h>

static uint16_t next = 5;

int tss_init(tss_t *tss, uint64_t rsp0)
{
    uint64_t low, high, addr;
    uint16_t cid;

    low = 0x0000E90000000000;
    high = 0x0000000000000000;
    addr = (uint64_t)tss;

    low |= sizeof(tss_t) - 1;
    low |= (addr & 0x00FFFFFF) << 16;
    low |= (addr & 0xFF000000) << 32;
    high |= (addr >> 32);

    memset(tss, 0, sizeof(tss_t));
    tss->rsp0 = rsp0;
    tss->iomap = sizeof(tss_t);

    gdt_set_gate(next+0, low);
    gdt_set_gate(next+1, high);

    cid = 8*next;
    asm volatile("ltr %%ax" :: "a" (cid));
    next += 2;

    return cid;
}
