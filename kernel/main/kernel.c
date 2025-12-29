#include <loader/bootstruct.h>
#include <kernel/sched/kthreads.h>
#include <kernel/sched/scheduler.h>
#include <kernel/sched/execve.h>
#include <kernel/term/console.h>
#include <kernel/term/term.h>
#include <kernel/input/input.h>
#include <kernel/acpi/acpi.h>
#include <kernel/x86/strace.h>
#include <kernel/x86/ioapic.h>
#include <kernel/x86/lapic.h>
#include <kernel/x86/cpuid.h>
#include <kernel/x86/idt.h>
#include <kernel/x86/isr.h>
#include <kernel/x86/gdt.h>
#include <kernel/x86/irq.h>
#include <kernel/x86/smp.h>
#include <kernel/x86/fpu.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/slmm.h>
#include <kernel/mem/e820.h>
#include <kernel/mem/mtrr.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/vmm.h>
#include <kernel/time/time.h>
#include <kernel/pci/pci.h>
#include <kernel/vfs/initrd.h>
#include <kernel/vfs/vfs.h>
#include <kernel/debug.h>
#include <string.h>
#include <stdio.h>

static char *getstr(const char *data, const char *name, char *str)
{
    const char *d;
    char *s;
    char buf[32];
    size_t len;

    len = sprintf(buf, "%s=", name);
    data = strstr(data, buf);
    if(!data)
    {
        return 0;
    }

    d = data + len;
    s = str;

    while(*d && *d != ' ')
    {
        *s++ = *d++;
    }

    *s = '\0';
    return str;
}

static void system_mount(const char *cmdline)
{
    char device[64];
    char fstype[16];

    if(getstr(cmdline, "device", device) == NULL)
    {
        return;
    }

    if(getstr(cmdline, "fstype", fstype) == NULL)
    {
        return;
    }

    if(vfs_mount(device, fstype, "system") < 0)
    {
        kp_error("main", "failed to mount %s", device);
    }
}

static void spawn_init()
{
    pid_t pid;
    int fd;

    fd = vfs_open("/devices/console", O_WRITE);
    if(fd < 0)
    {
        kp_crit("main", "failed to open /devices/console");
    }

    pid = execve("/initrd/apps/init.elf", 0, 0, fd, fd);
    if(pid < 0)
    {
        vfs_close(fd);
        kp_crit("main", "failed to spawn /initrd/apps/init.elf");
    }
}

void kmain(bootstruct_t *bs)
{
    // Descriptor tables
    gdt_init();
    idt_init();

    // Exceptions and stack tracing
    isr_init();
    strace_init(bs->symtab_address, bs->symtab_size);

    // The SLMM (Simple Linear Memory Manager) must be initialized before the console
    slmm_init(bs->kernel_address, bs->end_address);
    console_init(bs);

    // Debug information
    kp_info("boot", "signature: %s", bs->signature);
    kp_info("boot", "command line: %s", bs->cmdline);
    kp_info("boot", "lower memory: %d kb", bs->lower_memory/1024);
    kp_info("boot", "upper memory: %d kb", bs->upper_memory/1024);

    if(bs->lfb_mode == 0)
    {
        kp_info("boot", "text mode: %dx%d", bs->lfb_width, bs->lfb_height);
    }
    else
    {
        kp_info("boot", "graphical mode: %dx%dx%d", bs->lfb_width, bs->lfb_height, bs->lfb_bpp);
        kp_info("boot", "framebuffer: %#lx", bs->lfb_address);
    }

    // CPUID is used by several other modules to check for supported features
    cpuid_init();

    // Physical memory
    e820_init(bs->mmap_address, bs->mmap_size);
    mtrr_init();
    pmm_init();

    // Virtual memory
    // Once the VMM is initialized:
    //  - The console must be reinitialized to use a virtual address for the framebuffer
    //  - The SLMM must not be called anymore, so we initialize the heap
    pat_init();
    vmm_init();
    console_init(0);
    heap_init();

    // Information from the ACPI tables are needed by several other modules
    acpi_init(bs->rsdp_address);

    // Interrupt handling
    lapic_init();
    ioapic_init();
    irq_init();
    fpu_init();
    sti();

    // Time
    time_init();

    // File systems
    vfs_init();
    initrd_init(bs->initrd_address, bs->initrd_size);

    // Multitasking
    kthreads_init();
    smp_init();
    scheduler_init();

    // Subsystems
    input_init();
    term_init();

    // PCI
    pci_init();
    acpi_enable();
    pci_route_init();

    // start init process
    system_mount(bs->cmdline);
    term_switch(1);
    spawn_init();

    // Done
    thread_exit();
    while(1)
    {
        asm("hlt");
    }
}
