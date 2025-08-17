#include <kernel/acpi/mcfg.h>

static mcfg_t *mcfg = 0;
static uint8_t num_entries;

uint64_t acpi_mcfg_entries(int *num)
{
    if(mcfg == 0)
    {
        *num = 0;
        return 0;
    }
    else
    {
        *num = num_entries;
        return (uint64_t)mcfg->entry;
    }
}

void acpi_mcfg_parse(uint64_t address)
{
    header_t *hdr;
    mcfg = (mcfg_t*)address;
    hdr = (header_t*)address;
    num_entries = (hdr->length - sizeof(mcfg_t)) / sizeof(mcfg_entry_t);
}
