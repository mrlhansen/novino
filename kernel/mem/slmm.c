#include <kernel/mem/slmm.h>
#include <kernel/mem/vmm.h>
#include <kernel/mem/pmm.h>

static size_t slmm_start;
static size_t slmm_end;

size_t slmm_kmalloc(size_t size, int align)
{
    size_t addr;

    if(align && (slmm_end & ALIGN_TEST))
    {
        slmm_end &= ALIGN_MASK;
        slmm_end += PAGE_SIZE;
    }

    addr = slmm_end;
    slmm_end += size;
    slmm_report_reserved(0);

    return addr;
}

void slmm_report_reserved(int ready)
{
    static int pmm_ready = 0;
    size_t phys;

    if(ready)
    {
        pmm_ready = 1;
    }

    if(pmm_ready)
    {
        phys = (slmm_start & 0x0FFFFFFF);
        pmm_set_reserved(phys, slmm_end-slmm_start);
        slmm_start = slmm_end;
    }
}

void slmm_init(size_t start, size_t end)
{
    slmm_start = start;
    slmm_end = end;
}
