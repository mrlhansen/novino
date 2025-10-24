#include <kernel/sched/process.h>
#include <kernel/term/console.h>
#include <kernel/term/fbmem.h>
#include <kernel/x86/lapic.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/map.h>
#include <kernel/debug.h>

static uintptr_t phys = 0;
static uintptr_t virt = 0;
static process_t *pr = 0;
static vts_t *vts = 0;

int fbmem_open()
{
    return 0;
}

int fbmem_close()
{
    if(!pr)
    {
        return 0;
    }

    mm_unmap(&pr->mmap, virt);
    virt = 0;
    pr = 0;

    console_clear(vts->console);
    console_refresh(vts->console);

    return 0;
}

int fbmem_ioctl(fbmem_t *fb)
{
    console_t *con;

    pr = process_handle();
    con = vts->console;

    if(con->active)
    {
        mm_map_direct(&pr->mmap, con->lfbphys, con->memsz, MAP_WC, &virt);
        // handle errors
    }
    else
    {
        mm_map_direct(&pr->mmap, phys, con->memsz, MAP_WC, &virt);
        // handle errors
    }

    fb->addr = virt;
    fb->width = con->width;
    fb->height = con->height;
    fb->bpp = con->depth;
    fb->pitch = con->bytes_per_line;

    return 0;
}

int fbmem_toggle(bool active)
{
    uint32_t flags;
    uint64_t pml4;

    if(!pr)
    {
        return 0;
    }

    disable_interrupts(&flags);

    // switch address space
    pml4 = vmm_get_current_pml4();
    if(pml4 != pr->pml4)
    {
        vmm_load_pml4(pr->pml4);
    }

    // remap memory
    if(active)
    {
        mm_remap_direct(&pr->mmap, virt, vts->console->lfbphys);
    }
    else
    {
        mm_remap_direct(&pr->mmap, virt, phys);
    }

    // flush tlb
    vmm_load_pml4(pml4);
    lapic_bcast_ipi(254, false, true);

    restore_interrupts(&flags);
    return 0;
}

void fbmem_init(vts_t *fbvts)
{
    console_t *def, *con;

    vts = fbvts;
    def = console_default();
    con = kmalloc(sizeof(console_t));
    *con = *def;

    phys = 1 + (def->memsz / PAGE_SIZE);
    phys = pmm_alloc_frames(phys, 1);

    con->bufaddr = vmm_phys_to_virt(phys);
    con->active = 0;
    con->flags = 0;
    vts->flags = 0;
    vts->console = con;

    console_clear(con);
}
