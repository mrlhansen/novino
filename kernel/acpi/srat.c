#include <kernel/acpi/srat.h>

static srat_t *srat = 0;
static uint16_t entry_count;
static uint64_t entry_start;
static uint64_t entry_end;

void acpi_srat_parse(uint64_t address)
{
    srat_apic_t *entry;
    header_t *hdr;
    uint64_t ptr;

    srat = (srat_t*)address;
    hdr = (header_t*)address;

    entry_start = address + sizeof(srat_t);
    entry_end = address + hdr->length;
    entry_count = 0;

    ptr = entry_start;

    while(ptr < entry_end)
    {
        entry = (srat_apic_t*)ptr;
        ptr += entry->length;
        entry_count++;
    }
}
