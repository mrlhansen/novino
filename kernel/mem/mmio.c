#include <kernel/mem/vmm.h>
#include <kernel/mem/pmm.h>
#include <kernel/errno.h>

static uint64_t mmio_offset = 0;

int mmio_alloc_uc_region(uint64_t size, uint32_t align, uint64_t *virt, uint64_t *phys)
{
    uint64_t va, pa;
    uint32_t offset;

    // Check arguments
    if(virt == 0)
    {
        return -EINVAL;
    }

    if(phys == 0)
    {
        return -EINVAL;
    }

    if(size == 0)
    {
        return -EINVAL;
    }

    if(align == 0)
    {
        return -EINVAL;
    }

    // Translate to number of pages
    if(size & ALIGN_TEST)
    {
        size = (size & ALIGN_MASK) + PAGE_SIZE;
    }

    if(align & ALIGN_TEST)
    {
        align = (align & ALIGN_MASK) + PAGE_SIZE;
    }

    size /= PAGE_SIZE;
    align /= PAGE_SIZE;

    // Allocate physical memory
    pa = pmm_alloc_frames(size, align);

    if(pa == 0)
    {
        return -ENOMEM;
    }

    va = HWMAP + mmio_offset;
    mmio_offset += (size * PAGE_SIZE);
    offset = 0;

    // Map virtual memory
    for(int n = 0; n < size; n++)
    {
        if(vmm_map_page(va + offset, pa + offset))
        {
            return -EFAIL;
        }

        vmm_set_caching(va + offset, PAT_UC);
        offset += PAGE_SIZE;
    }

    // Store results
    *virt = va;
    *phys = pa;

    // Success
    return 0;
}

int mmio_map_wc_region(uint64_t addr, uint64_t size, uint64_t *virt)
{
    uint64_t va, pa;
    uint32_t offset;

    // Check arguments
    if(virt == 0)
    {
        return -EINVAL;
    }

    if(addr == 0)
    {
        return -EINVAL;
    }

    if(size == 0)
    {
        return -EINVAL;
    }

    // Translate to number of pages
    if(addr & ALIGN_TEST)
    {
        pa = (addr & ALIGN_MASK);
        size = size + (addr - pa);
    }
    else
    {
        pa = addr;
    }

    if(size & ALIGN_TEST)
    {
        size = (size & ALIGN_MASK) + PAGE_SIZE;
    }

    size /= PAGE_SIZE;

    // Map virtual memory
    va = HWMAP + mmio_offset;
    mmio_offset += (size * PAGE_SIZE);
    offset = 0;

    // Map virtual memory
    for(int n = 0; n < size; n++)
    {
        if(vmm_map_page(va + offset, pa + offset))
        {
            return -EFAIL;
        }

        vmm_set_caching(va + offset, PAT_WC);
        offset += PAGE_SIZE;
    }

    *virt = va + (addr - pa);

    // Success
    return 0;
}
