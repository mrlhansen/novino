#include <kernel/acpi/acpi.h>
#include <kernel/time/time.h>
#include <kernel/x86/lapic.h>
#include <kernel/mem/vmm.h>
#include <kernel/debug.h>

static uint64_t address;
static uint8_t version;
static uint8_t max_lvt;

uint32_t lapic_read(uint16_t reg)
{
    return *(volatile uint32_t*)(address + reg);
}

void lapic_write(uint16_t reg, uint32_t value)
{
    *(volatile uint32_t*)(address + reg) = value;
}

uint8_t lapic_get_id()
{
    return (lapic_read(APIC_ID) >> 24);
}

void lapic_ipi(uint32_t apic_id, uint32_t type, uint8_t vector)
{
    lapic_write(APIC_ICR1, apic_id << 24);
    lapic_write(APIC_ICR0, (0x4000 | type | vector));
}

void lapic_bcast_ipi(uint8_t vector, bool self, bool all)
{
    uint32_t icr0;

    icr0 = (0x4000 | vector);
    if(all)
    {
        if(self)
        {
            icr0 |= (2 << 18);
        }
        else
        {
            icr0 |= (3 << 18);
        }
    }
    else
    {
        icr0 |= (1 << 18);
    }

    lapic_write(APIC_ICR1, 0);
    lapic_write(APIC_ICR0, icr0);
}

void lapic_timer_mask()
{
    uint32_t value;
    value = lapic_read(APIC_LVTT);
    value |= (1 << 16);
    lapic_write(APIC_LVTT, value);
}

void lapic_timer_unmask()
{
    uint32_t value;
    value = lapic_read(APIC_LVTT);
    value &= ~(1 << 16);
    lapic_write(APIC_LVTT, value);
}

void lapic_timer_init(uint32_t count, bool periodic)
{
    if(periodic)
    {
        lapic_write(APIC_LVTT, 0x20020);
    }
    else
    {
        lapic_write(APIC_LVTT, 0x20);
    }
    lapic_write(APIC_TDCR, 0x02);
    lapic_write(APIC_TICR, count);
}

uint64_t lapic_timer_calibrate()
{
    uint32_t count;

    // Divide by 8
    lapic_write(APIC_TDCR, 0x02);

    // Wait 10 ms
    timer_wait();
    lapic_write(APIC_TICR, 0xFFFFFFFF);
    timer_sleep(10);

    // Number of ticks
    count = lapic_read(APIC_TCCR);
    count = 0xFFFFFFFF - count;

    // Return frequency
    return (100UL * count);
}

void lapic_enable()
{
    // Enable, spurious vector = 255
    lapic_write(APIC_SIVR, 0x1FF);

    // Allow all interrupts
    lapic_write(APIC_TPR, 0);
}

void lapic_init()
{
    uint64_t phys;
    uint32_t value;

    // Address from ACPI tables
    phys = acpi_lapic_address();
    address = vmm_phys_to_virt(phys);

    // Version and Max LVT entries
    value = lapic_read(APIC_VER);
    version = (value & 0xFF);
    max_lvt = ((value >> 16) & 0xFF) + 1;
    kp_info("apic", "lapic: %#08lx, max lvt: %d, version: %#x", phys, max_lvt, version);

    // Enable
    lapic_enable();
}
