#include <kernel/sched/scheduler.h>
#include <kernel/acpi/acpi.h>
#include <kernel/mem/vmm.h>
#include <kernel/x86/lapic.h>
#include <kernel/x86/smp.h>
#include <kernel/x86/gdt.h>
#include <kernel/x86/idt.h>
#include <kernel/x86/irq.h>
#include <kernel/x86/tss.h>
#include <kernel/x86/fpu.h>
#include <kernel/debug.h>
#include <string.h>

static core_t core[16];
static uint8_t core_count;
static volatile int ap_booted;
static volatile int core_id;

int smp_core_apic_id(int id)
{
    if(core_count == 0)
    {
        return lapic_get_id();
    }
    else
    {
        id = (id % core_count);
        return core[id].apic_id;
    }
}

int smp_core_id()
{
    uint32_t ax;
    asm volatile("str %%ax" : "=a" (ax));
    ax = (ax - core[0].tr) / 0x10;
    return ax;
}

int smp_core_count()
{
    return core_count;
}

void smp_enable_core(int apic_id)
{
    uint8_t id;
    id = core_count++;

    core[id].present = 1;
    core[id].apic_id = apic_id;

    if(lapic_get_id() == apic_id)
    {
        core[id].bsp = 1;
        kp_info("smp", "apic_id: %d (bsp)", apic_id);
    }
    else
    {
        core[id].bsp = 0;
        kp_info("smp", "apic_id: %d (ap)", apic_id);
    }
}

static void smp_init_core(int id)
{
    core[id].tr = tss_init(&core[id].tss, 0);
    scheduler_init_core(core_count, id, core[id].apic_id, &core[id].tss);
    fpu_init();
    syscall_init();
}

void smp_ap_entry()
{
    gdt_load();
    idt_load();
    vmm_load_kernel_pml4();
    lapic_enable();
    smp_init_core(core_id);
    sti();

    ap_booted = 1;

    while(1)
    {
        asm volatile("hlt");
    }
}

static void smp_wait()
{
    int i = 10000;
    while(i--) asm volatile("nop");
}

static void smp_copy_boot_code()
{
    void *dptr, *sptr;
    dptr = (void*)vmm_phys_to_virt(0x1000);
    sptr = (void*)ap_boot_code;
    memcpy(dptr, sptr, ap_boot_size);
}

static int smp_boot_ap(uint8_t id)
{
    ap_booted = 0;
    core_id = id;

    lapic_ipi(core[id].apic_id, 0x500, 0x01); // Send INIT IPI
    smp_wait();
    lapic_ipi(core[id].apic_id, 0x600, 0x01); // Send SIPI IPI
    smp_wait();

    while(ap_booted == 0);
    return 0;
}

void smp_init()
{
    // Register all available cores
    core_count = 0;
    acpi_report_core();

    // Copy AP boot code to low memory
    smp_copy_boot_code();

    // Boot all AP cores
    for(int id = 0; id < core_count; id++)
    {
        if(core[id].bsp == 0)
        {
            if(smp_boot_ap(id) == 0)
            {
                kp_info("smp", "succesfully booted core %d", core[id].apic_id);
            }
        }
        else
        {
            smp_init_core(id);
        }
    }
}
