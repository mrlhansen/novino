#include <kernel/acpi/acpi.h>
#include <kernel/acpi/madt.h>
#include <kernel/x86/irq.h>
#include <kernel/x86/ioapic.h>
#include <kernel/x86/smp.h>

static madt_t *madt = 0;
static uint64_t lapic_address = 0;

static uint16_t entry_count;
static uint64_t entry_start;
static uint64_t entry_end;

void acpi_report_iso()
{
    madt_iso_t *entry;
    uint64_t ptr = entry_start;

    uint8_t modes[4] = {0, 0, 0, 1};
    uint8_t polarity;
    uint8_t triggered;

    if(madt == 0)
    {
        return;
    }

    while(ptr < entry_end)
    {
        entry = (madt_iso_t*)ptr;
        if(entry->type == MADT_ISO)
        {
            polarity = modes[entry->polarity];
            triggered = modes[entry->triggered];
            irq_configure(entry->source, entry->gsi, polarity, triggered);
        }
        ptr += entry->length;
    }
}

void acpi_report_ioapic()
{
    madt_ioapic_t *entry;
    uint64_t ptr = entry_start;

    if(madt == 0)
    {
        return;
    }

    while(ptr < entry_end)
    {
        entry = (madt_ioapic_t*)ptr;
        if(entry->type == MADT_IOAPIC)
        {
            ioapic_enable(entry->ioapic_id, entry->gsi_base, entry->ioapic_address);
        }
        ptr += entry->length;
    }
}

void acpi_report_core()
{
    madt_lapic_t *entry;
    uint64_t ptr = entry_start;

    if(madt == 0)
    {
        return;
    }

    while(ptr < entry_end)
    {
        entry = (madt_lapic_t*)ptr;
        if(entry->type == MADT_LAPIC)
        {
            if(entry->flags & 0x01)
            {
                smp_enable_core(entry->lapic_id);
            }
        }
        ptr += entry->length;
    }
}

uint64_t acpi_lapic_address()
{
    return lapic_address;
}

void acpi_madt_parse(uint64_t address)
{
    madt_lapic_ao_t *entry;
    header_t *hdr;
    uint64_t ptr;

    madt = (madt_t*)address;
    hdr = (header_t*)address;

    entry_start = address + sizeof(madt_t);
    entry_end = address + hdr->length;
    entry_count = 0;

    ptr = entry_start;
    lapic_address = madt->lapic_address;

    while(ptr < entry_end)
    {
        entry = (madt_lapic_ao_t*)ptr;
        if(entry->type == MADT_LAPIC_AO)
        {
            lapic_address = entry->lapic_address;
        }
        ptr += entry->length;
        entry_count++;
    }
}
