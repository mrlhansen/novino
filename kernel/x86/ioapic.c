#include <kernel/acpi/acpi.h>
#include <kernel/x86/ioports.h>
#include <kernel/x86/ioapic.h>
#include <kernel/x86/lapic.h>
#include <kernel/mem/vmm.h>
#include <kernel/debug.h>

#define MAX_IOAPIC 4
static ioapic_t ioapic[MAX_IOAPIC];
static int ioapic_count = 0;
int ioapic_max_gsi = 0;

static uint32_t ioapic_read(uint8_t id, uint16_t reg)
{
    volatile uint32_t *memptr = (volatile uint32_t*)ioapic[id].address;
    memptr[0] = reg;
    return memptr[4];
}

static void ioapic_write(uint8_t id, uint16_t reg, uint32_t value)
{
    volatile uint32_t *memptr = (volatile uint32_t*)ioapic[id].address;
    memptr[0] = reg;
    memptr[4] = value;
}

static int ioapic_find(uint32_t gsi, uint8_t *id, uint8_t *pin)
{
    int start, end;

    for(int n = 0; n < ioapic_count; n++)
    {
        start = ioapic[n].gsi_base;
        end = start + ioapic[n].max_entries;
        if(gsi >= start && gsi < end)
        {
            *id = n;
            *pin = gsi - start;
            return 0;
        }
    }

    return 1;
}

void ioapic_enable(uint8_t apic_id, uint32_t gsi_base, uint64_t address)
{
    uint32_t value, id;
    id = ioapic_count++;

    ioapic[id].apic_id = apic_id;
    ioapic[id].address = vmm_phys_to_virt(address);
    ioapic[id].gsi_base = gsi_base;

    value = ioapic_read(id, IOAPIC_VER);
    ioapic[id].version = (value & 0xFF);
    ioapic[id].max_entries = ((value >> 16) & 0xFF) + 1;

    value = gsi_base + ioapic[id].max_entries;
    if(value > ioapic_max_gsi)
    {
        ioapic_max_gsi = value;
    }

    kp_info("apic", "ioapic: %#08lx, apic id: %d, gsi: %d-%d, version: %#x", address, apic_id, gsi_base, value-1, ioapic[id].version);
}

void ioapic_route(uint32_t gsi, redir_t *redir)
{
    uint32_t upper, lower, reg;
    uint8_t id, pin;
    uint64_t entry;

    if(ioapic_find(gsi, &id, &pin))
    {
        return;
    }

    entry = *(uint64_t*)redir;
    lower = (entry & 0xFFFFFFFF);
    upper = (entry >> 32);

    reg = IOAPIC_REDTBL + (pin << 1);
    ioapic_write(id, reg+0, lower);
    ioapic_write(id, reg+1, upper);
}

void ioapic_init()
{
    // Remap the legacy PIC
    outportb(0x20, 0x11);
    outportb(0xA0, 0x11);
    outportb(0x21, 0x20); // Offset for irq 0-7
    outportb(0xA1, 0x28); // Offset for irq 8-15
    outportb(0x21, 0x04);
    outportb(0xA1, 0x02);
    outportb(0x21, 0x01);
    outportb(0xA1, 0x01);
    outportb(0x21, 0xFF); // Mask irq 0-7
    outportb(0xA1, 0xFF); // Mask irq 8-15

    // Information from ACPI tables
    acpi_report_ioapic();

    // Send EOI to local APIC to clear things out
    lapic_write(APIC_EOI, 0);
}
