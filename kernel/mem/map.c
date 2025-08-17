#include <kernel/mem/heap.h>
#include <kernel/mem/map.h>
#include <kernel/mem/vmm.h>
#include <kernel/errno.h>
#include <kernel/debug.h>

// TODO: for security reasons we should zero out all memory made available to user space
// this is relevant in several places, such as brk syscall

static mm_region_t *find_free_region(list_t *map, size_t length)
{
    mm_region_t *curr = map->head;
    mm_region_t *best = 0;

    while(curr)
    {
        if((curr->flags & MAP_FREE) == 0)
        {
            continue;
        }

        if(curr->length < length)
        {
            continue;
        }

        if(best)
        {
            if(curr->length < best->length)
            {
                best = curr;
            }
        }
        else
        {
            best = curr;
        }

        curr = curr->link.next;
    }

    return best;
}

static mm_region_t *find_region(list_t *map, size_t addr)
{
    mm_region_t *curr = map->head;
    size_t start, end;

    while(curr)
    {
        start = curr->addr;
        end = start + curr->length;

        if(addr >= start && addr < end)
        {
            return curr;
        }

        curr = curr->link.next;
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

    list_insert(map, new);

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
        return -1; // failed, need rollback
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
        return -1; // failed, need rollback
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

int mm_unmap(size_t addr)
{
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

    status = map_region(r);
    if(status < 0)
    {
        return status; // region should be merged again
    }

    r->phys = phys;
    r->flags = flags | MAP_DIRECT;
    *virt = r->addr;

    return 0;
}

// int mm_remap_direct(list_t *map, size_t virt, size_t phys)
// {
//     mm_region_t *r;

//     r = find_region(map, virt);
//     if(r == 0)
//     {
//         return -EINVAL;
//     }

//     if((r->flags & MAP_DIRECT) == 0)
//     {
//         return -EINVAL;
//     }

//     r->phys = phys;
//     map_region(r);

//     return 0;
// }

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
