#include <kernel/acpi/fadt.h>
#include <kernel/x86/irq.h>
#include <kernel/debug.h>
#include <sabi/events.h>
#include <sabi/pm.h>

static void acpi_handler(int gsi, void *data)
{
    int flags;
    flags = sabi_read_event();

    kp_info("acpi", "handler called, event %x", flags);

    if(flags & ACPI_POWER_BUTTON)
    {
        sabi_pm_soft_off();
    }
}

void acpi_enable()
{
    int gsi, vector, status;
    fadt_t *fadt;

    fadt = acpi_fadt_table();
    gsi = fadt->sci_int;

    status = sabi_enable_acpi(1);
    if(status < 0)
    {
        kp_info("acpi", "unable to enable acpi");
        return;
    }

    vector = irq_alloc_gsi_vector(gsi);
    irq_request(vector, acpi_handler, fadt);
    kp_info("acpi", "acpi enabled, gsi %d", gsi);
}
