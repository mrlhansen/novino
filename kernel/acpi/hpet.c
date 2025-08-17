#include <kernel/acpi/hpet.h>
#include <kernel/debug.h>

static hpet_t *hpet = 0;

uint64_t acpi_hpet_address()
{
    if(hpet)
    {
        return hpet->address.address;
    }
    else
    {
        return 0;
    }
}

void acpi_hpet_parse(uint64_t address)
{
    hpet = (hpet_t*)address;
}
