#include <kernel/acpi/fadt.h>
#include <kernel/x86/ioports.h>
#include <sabi/api.h>

static fadt_t *fadt = 0;

fadt_t *acpi_fadt_table()
{
    return fadt;
}

int acpi_fadt_flag(uint32_t flag)
{
    if(fadt->flags & (1 << flag))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int acpi_8042_support()
{
    if(fadt == 0)
    {
        return 1;
    }

    if(fadt->hdr.revision == 1)
    {
        return 1;
    }

    if(fadt->iapc_boot_arch & 0x02)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void acpi_reboot()
{
    uint8_t type;
    uint8_t *ptr;

    // Check for support via flags
    if(acpi_fadt_flag(RESET_REG_SUP) == 0)
    {
        return;
    }

    // Reset method
    type = fadt->reset_reg.address_space_id;

    // Reset using memory address (must be mapped!)
    if(type == GAS_SYSTEM_MEM)
    {
        ptr = (uint8_t*)fadt->reset_reg.address;
        *ptr = fadt->reset_value;
    }

    // Reset using I/O bus
    if(type == GAS_SYSTEM_IO)
    {
        outportb(fadt->reset_reg.address, fadt->reset_value);
    }

    // Reset using PCI bus
    if(type == GAS_PCI_CFG_SPACE)
    {
        // Missing
    }
}

void acpi_fadt_parse(uint64_t address)
{
    fadt = (fadt_t*)address;
    sabi_register_table(address);
}

void acpi_ssdt_parse(uint64_t address)
{
    sabi_register_table(address);
}
