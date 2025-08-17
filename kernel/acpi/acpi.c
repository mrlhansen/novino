#include <kernel/acpi/madt.h>
#include <kernel/acpi/hpet.h>
#include <kernel/acpi/mcfg.h>
#include <kernel/acpi/fadt.h>
#include <kernel/acpi/srat.h>
#include <kernel/mem/vmm.h>
#include <kernel/debug.h>
#include <string.h>

static rsdt_t *rsdt = 0;
static xsdt_t *xsdt = 0;
static int num_tables = 0;

static void acpi_print_table(const char *name, header_t *hdr)
{
    kp_info("acpi", "%.4s: %#016lx, size: %05d, revision: %d, oem: %.6s", name, hdr, hdr->length, hdr->revision, hdr->oem_id);
}

static int acpi_checksum(void *ptr, int size)
{
    uint8_t checksum = 0;
    uint8_t *cptr = (uint8_t*)ptr;

    for(int i = 0; i < size; i++)
    {
        checksum += cptr[i];
    }

    return checksum;
}

static void acpi_rsdt(uint64_t address)
{
    header_t *hdr;

    address = vmm_phys_to_virt(address);
    rsdt = (rsdt_t*)address;
    hdr = (header_t*)address;
    num_tables = (hdr->length - sizeof(header_t)) / sizeof(uint32_t);

    if(acpi_checksum(hdr, hdr->length))
    {
        kp_panic("acpi", "rsdt has invalid checksum");
        return;
    }

    acpi_print_table("rsdt", hdr);
}

static void acpi_xsdt(uint64_t address)
{
    header_t *hdr;

    address = vmm_phys_to_virt(address);
    xsdt = (xsdt_t*)address;
    hdr = (header_t*)address;
    num_tables = (hdr->length - sizeof(header_t)) / sizeof(uint64_t);

    if(acpi_checksum(hdr, hdr->length))
    {
        kp_panic("acpi", "xsdt has invalid checksum");
        return;
    }

    acpi_print_table("xsdt", hdr);
}

static void acpi_parse_tables()
{
    header_t *hdr;
    uint64_t address;

    for(int n = 0; n < num_tables; n++)
    {
        if(rsdt)
        {
            address = rsdt->table_ptr[n];
            address = vmm_phys_to_virt(address);
            hdr = (header_t*)address;
        }
        else
        {
            address = xsdt->table_ptr[n];
            address = vmm_phys_to_virt(address);
            hdr = (header_t*)address;
        }

        if(strncmp(hdr->signature, "APIC", 4) == 0)
        {
            if(acpi_checksum(hdr, hdr->length))
            {
                kp_warn("acpi", "madt table has invalid checksum");
            }
            else
            {
                acpi_print_table("madt", hdr);
                acpi_madt_parse(address);
            }
        }
        else if(strncmp(hdr->signature, "HPET", 4) == 0)
        {
            if(acpi_checksum(hdr, hdr->length))
            {
                kp_warn("acpi", "hpet table has invalid checksum");
            }
            else
            {
                acpi_print_table("hpet", hdr);
                acpi_hpet_parse(address);
            }
        }
        else if(strncmp(hdr->signature, "MCFG", 4) == 0)
        {
            if(acpi_checksum(hdr, hdr->length))
            {
                kp_warn("acpi", "mcfg table has invalid checksum");
            }
            else
            {
                acpi_print_table("mcfg", hdr);
                acpi_mcfg_parse(address);
            }
        }
        else if(strncmp(hdr->signature, "FACP", 4) == 0)
        {
            if(acpi_checksum(hdr, hdr->length))
            {
                kp_warn("acpi", "fadt table has invalid checksum");
                continue;
            }
            else
            {
                acpi_print_table("fadt", hdr);
                acpi_fadt_parse(address);
            }
        }
        else if(strncmp(hdr->signature, "SRAT", 4) == 0)
        {
            if(acpi_checksum(hdr, hdr->length))
            {
                kp_warn("acpi", "fadt table has invalid checksum");
                continue;
            }
            else
            {
                acpi_print_table("srat", hdr);
                acpi_srat_parse(address);
            }
        }
        else if(strncmp(hdr->signature, "SSDT", 4) == 0)
        {
            if(acpi_checksum(hdr, hdr->length))
            {
                kp_warn("acpi", "fadt table has invalid checksum");
                continue;
            }
            else
            {
                acpi_print_table("ssdt", hdr);
                acpi_ssdt_parse(address);
            }
        }
        else
        {
            char name[5] = {0};
            strncpy(name, hdr->signature, 4);
            strtolower(name);
            acpi_print_table(name, hdr);
        }
    }
}

void acpi_init(uint64_t address)
{
    rsdp_t *rsdp = (rsdp_t*)address;

    if(rsdp == 0)
    {
        kp_panic("acpi", "unable to find acpi tables");
    }

    if(acpi_checksum(rsdp, rsdp->length))
    {
        kp_panic("acpi", "rsdp has invalid checksum");
        return;
    }

    kp_info("acpi", "rsdp: %#016lx, size: %05d, revision: %d, oem: %.6s", address, rsdp->length, rsdp->revision, rsdp->oem_id);

    if(rsdp->revision == 0)
    {
        acpi_rsdt(rsdp->rsdt_address); // ACPI 1.0
    }
    else
    {
        acpi_xsdt(rsdp->xsdt_address); // ACPI 2.0+
    }

    acpi_parse_tables();
}
