#include <kernel/mem/heap.h>
#include <kernel/mem/map.h>
#include <kernel/mem/vmm.h>
#include <kernel/errno.h>
#include <kernel/debug.h>

// TODO: for security reasons we should zero out all memory made available to user space
// this is relevant in several places, such as brk syscall

static mm_region_t *find_free_region(list_t *map, size_t length)
{
    mm_region_t *item = 0;
    mm_region_t *best = 0;

    while(item = list_iterate(map, item), item)
    {
        if((item->flags & MAP_FREE) == 0)
        {
            continue;
        }

        if(item->length < length)
        {
            continue;
        }

        if(best)
        {
            if(item->length < best->length)
            {
                best = item;
            }
        }
        else
        {
            best = item;
        }
    }

    return best;
}

static mm_region_t *find_region(list_t *map, size_t addr)
{
    mm_region_t *item = 0;
    size_t start, end;

    while(item = list_iterate(map, item), item)
    {
        start = item->addr;
        end = start + item->length;
        if(addr >= start && addr < end)
        {
            return item;
        }
    }

    return 0;
}

static int split_region(list_t *map, mm_region_t *r, size_t length)
{
    mm_region_t *new;

    if(r->length - length < PAGE_SIZE)
    {
        return 0;
    }

    new = kzalloc(sizeof(mm_region_t));
    if(new == 0)
    {
        return -ENOMEM;
    }

    new->flags = MAP_FREE;
    new->addr = r->addr + length;
    new->length = r->length - length;

    list_insert_after(map, r, new);

    r->flags = 0;
    r->length = length;

    return 0;
}

static int map_region(mm_region_t *r)
{
    size_t offset = 0;
    size_t addr;
    int w, x;

    while(offset < r->length)
    {
        addr = r->addr + offset;

        if(vmm_map_page(addr, r->phys + offset) < 0)
        {
            break;
        }

        if(r->flags & MAP_UC)
        {
            vmm_set_caching(addr, PAT_UC);
        }
        else if(r->flags & MAP_WC)
        {
            vmm_set_caching(addr, PAT_WC);
        }

        x = (r->flags & MAP_EXEC) ? 1 : 0;
        w = (r->flags & MAP_READONLY) ? 0 : 1;
        vmm_set_mode(addr, w, x);

        offset += PAGE_SIZE;
    }

    if(offset < r->length)
    {
        kp_error("mmap", "failed to map region: %lx", r->addr);
        return -1; // failed, need rollback
    }

    return 0;
}

static int remap_region(mm_region_t *r)
{
    size_t offset = 0;
    size_t addr;

    while(offset < r->length)
    {
        addr = r->addr + offset;
        if(vmm_remap_page(addr, r->phys + offset) < 0)
        {
            break;
        }
        offset += PAGE_SIZE;
    }

    if(offset < r->length)
    {
        kp_error("mmap", "failed to remap region: %lx", r->addr);
        return -1;
    }

    return 0;
}

static int allocate_region(mm_region_t *r)
{
    size_t offset = 0;
    size_t addr;
    int w, x;

    while(offset < r->length)
    {
        addr = r->addr + offset;

        if(vmm_alloc_page(addr) < 0)
        {
            break;
        }

        if(r->flags & MAP_UC)
        {
            vmm_set_caching(addr, PAT_UC);
        }
        else if(r->flags & MAP_WC)
        {
            vmm_set_caching(addr, PAT_WC);
        }

        x = (r->flags & MAP_EXEC) ? 1 : 0;
        w = (r->flags & MAP_READONLY) ? 0 : 1;
        vmm_set_mode(addr, w, x);

        offset += PAGE_SIZE;
    }

    if(offset < r->length)
    {
        kp_error("mmap", "failed to allocate region: %lx", r->addr);
        return -1; // failed, need rollback
    }

    return 0;
}

static int unmap_region(mm_region_t *r)
{
    size_t offset = 0;
    size_t addr;

    while(offset < r->length)
    {
        addr = r->addr + offset;
        if(vmm_free_page(addr) < 0)
        {
            break;
        }
        offset += PAGE_SIZE;
    }

    if(offset < r->length)
    {
        kp_error("mmap", "failed to unmap region: %lx", r->addr);
        return -1; // this should never happen, but we keep the error in case of bugs
    }

    return 0;
}

int mm_map(list_t *map, size_t length, int flags, size_t *virt)
{
    mm_region_t *r;
    int status;

    length = PAGE_ALIGN(length);

    r = find_free_region(map, length);
    if(r == 0)
    {
        return -ENOMEM;
    }

    status = split_region(map, r, length);
    if(status < 0)
    {
        return status;
    }

    status = allocate_region(r);
    if(status < 0)
    {
        return status; // region should be merged again
    }

    r->flags = flags;
    *virt = r->addr;

    return 0;
}

int mm_unmap(list_t *map, size_t virt)
{
    mm_region_t *r;
    int status;

    r = find_region(map, virt);
    if(r == 0)
    {
        return -EINVAL;
    }

    status = unmap_region(r);
    if(status < 0)
    {
        return status;
    }

    // here we should merge regions instead
    r->flags = MAP_FREE;

    return 0;
}

int mm_map_direct(list_t *map, size_t phys, size_t length, int flags, size_t *virt)
{
    mm_region_t *r;
    int status;

    length = PAGE_ALIGN(length);

    r = find_free_region(map, length);
    if(r == 0)
    {
        return -ENOMEM;
    }

    status = split_region(map, r, length);
    if(status < 0)
    {
        return status;
    }

    r->phys = phys;
    r->flags = flags | MAP_DIRECT;

    status = map_region(r);
    if(status < 0)
    {
        return status; // region should be merged again
    }

    *virt = r->addr;
    return 0;
}

int mm_remap_direct(list_t *map, size_t virt, size_t phys)
{
    mm_region_t *r;

    r = find_region(map, virt);
    if(r == 0)
    {
        return -EINVAL;
    }

    if((r->flags & MAP_DIRECT) == 0)
    {
        return -EINVAL;
    }

    r->phys = phys;
    remap_region(r);

    return 0;
}

int mm_init(list_t *map, size_t addr, size_t length)
{
    mm_region_t *first;

    first = kzalloc(sizeof(mm_region_t));
    if(first == 0)
    {
        return -ENOMEM;
    }

    first->flags = MAP_FREE;
    first->addr = addr;
    first->length = length;

    list_init(map, offsetof(mm_region_t, link));
    list_insert(map, first);

    return 0;
}
