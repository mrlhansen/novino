#include <kernel/mem/heap.h>
#include <kernel/mem/slmm.h>
#include <kernel/mem/vmm.h>
#include <kernel/mem/pmm.h>
#include <kernel/debug.h>
#include <string.h>

static heap_t heap;

// HEAP_HEADER_SIZE must be an integer multiple of HEAP_DATA_ALIGN
// HEAP_DATA_ALIGN must be able to fit two pointers

#define HEAP_HEADER_SIZE  sizeof(chunk_t)
#define HEAP_DATA_ALIGN   (2*sizeof(size_t))
#define HEAP_SBRK_SIZE    0x100000

#define align_size(s,a)  ((s + a - 1) & -(a))
#define chunk_size(c)    ((size_t)c->next - (size_t)c - HEAP_HEADER_SIZE)
#define chunk_next(c,s)  ((void*)c + HEAP_HEADER_SIZE + s)

static void insert_free_chunk(chunk_t *chunk)
{
    chunk->free = 1;

    chunk[1].next = heap.free;
    chunk[1].prev = 0;

    if(heap.free)
    {
        heap.free[1].prev = chunk;
    }

    heap.free = chunk;
}

static void remove_free_chunk(chunk_t *chunk)
{
    chunk->free = 0;

    if(heap.free == chunk)
    {
        heap.free = chunk[1].next;
    }

    if(chunk[1].prev)
    {
        chunk[1].prev[1].next = chunk[1].next;
    }

    if(chunk[1].next)
    {
        chunk[1].next[1].prev = chunk[1].prev;
    }
}

static void split_chunk(chunk_t *chunk, size_t size)
{
    chunk_t *next;

    // check size
    if(chunk->size < size + HEAP_DATA_ALIGN + HEAP_HEADER_SIZE)
    {
        return;
    }

    // new free chunk
    next = chunk_next(chunk, size);
    next->prev = chunk;
    next->next = chunk->next;
    next->size = chunk_size(next);
    insert_free_chunk(next);

    // adjust chunk
    chunk->next->prev = next;
    chunk->next = next;
    chunk->size = size;
}

static void glue_chunk(chunk_t *chunk, int prev, int next)
{
    // backwards
    if(prev && chunk->prev->free)
    {
        remove_free_chunk(chunk);
        remove_free_chunk(chunk->prev);

        chunk->prev->size += chunk->size + HEAP_HEADER_SIZE;
        chunk->prev->next = chunk->next;
        chunk->next->prev = chunk->prev;

        insert_free_chunk(chunk->prev);
        chunk = chunk->prev;
    }

    // forwards
    if(next && chunk->next->free)
    {
        remove_free_chunk(chunk);
        remove_free_chunk(chunk->next);

        chunk->size += chunk->next->size + HEAP_HEADER_SIZE;
        chunk->next = chunk->next->next;
        chunk->next->prev = chunk;

        insert_free_chunk(chunk);
    }
}

static size_t heap_sbrk(size_t size)
{
    size_t addr, end;

    size = align_size(size, HEAP_SBRK_SIZE);
    addr = heap.start + heap.size;
    end = addr + size;
    size = 0;

    if(end > heap.start + KHEAP_SIZE)
    {
        return 0;
    }

    while(addr < end)
    {
        if(vmm_alloc_page(addr))
        {
            kp_warn("heap", "failed to expand heap size");
            break;
        }
        else
        {
            addr += PAGE_SIZE;
            size += PAGE_SIZE;
        }
    }

    heap.size += size;
    heap.end += size;

    return size;
}

static chunk_t *heap_expand(size_t size)
{
    chunk_t *old;
    chunk_t *new;

    // make room for header
    size = size + HEAP_HEADER_SIZE;

    // allocate memory
    old = (void*)(heap.end - HEAP_HEADER_SIZE);
    if(heap_sbrk(size) < size)
    {
        return 0;
    }
    new = (void*)(heap.end - HEAP_HEADER_SIZE);

    // turn old rear guard into free block
    old->next = new;
    old->size = chunk_size(old);
    old->free = 1;

    // new rear guard
    new->prev = old;
    new->next = 0;
    new->size = 0;
    new->free = 0;

    // insert
    insert_free_chunk(old);

    // merge when possible
    if(old->prev->free)
    {
        old = old->prev;
        glue_chunk(old, 0, 1);
    }

    return old;
}

static chunk_t *find_free_chunk(size_t size)
{
    chunk_t *chunk;

    // find in list
    chunk = heap.free;
    while(chunk)
    {
        if(chunk->size >= size)
        {
            return chunk;
        }
        chunk = chunk[1].next;
    }

    // try to allocate more memory
    return heap_expand(size);
}

static size_t kmalloc_int(size_t size)
{
    chunk_t *chunk;
    size_t addr;

    // always returns zero
    if(size == 0)
    {
        return 0;
    }

    if(heap.size)
    {
        // round size to alignment
        size = align_size(size, HEAP_DATA_ALIGN);

        // find a free chunk
        chunk = find_free_chunk(size);
        if(!chunk)
        {
            return 0;
        }
        addr = (size_t)(chunk+1);

        // split chunk if necessary
        split_chunk(chunk, size);

        // remove chunk from free list
        remove_free_chunk(chunk);
    }
    else
    {
        addr = slmm_kmalloc(size, 0);
    }

    return addr;
}

void *kmalloc(size_t size)
{
    return (void*)kmalloc_int(size);
}

void *kzalloc(size_t size)
{
    void *ptr = (void*)kmalloc_int(size);
    memset(ptr, 0, size);
    return ptr;
}

void *kcalloc(size_t count, size_t size)
{
    void *ptr = (void*)kmalloc_int(count*size);
    memset(ptr, 0, count*size);
    return ptr;
}

void kfree(void *ptr)
{
    chunk_t *chunk;

    // invalid
    if(ptr < (void*)heap.start)
    {
        return;
    }

    if(ptr > (void*)heap.end)
    {
        return;
    }

    // pointer to chunk
    chunk = ptr - HEAP_HEADER_SIZE;

    // insert as free
    insert_free_chunk(chunk);

    // glue chunks when possible
    glue_chunk(chunk, 1, 1);
}

size_t heap_get_size()
{
    return heap.size;
}

void heap_init()
{
    chunk_t *first, *last, *middle;

    // initialize heap
    heap.first = 0;
    heap.free = 0;
    heap.start = KHEAP;
    heap.end = KHEAP;
    heap.size = 0;

    // allocate initial memory
    heap_sbrk(1);

    // front guard block
    first = (void*)heap.start;
    first->prev = 0;
    first->next = first + 1;
    first->size = 0;
    first->free = 0;

    // rear guard block
    last = (void*)(heap.end - HEAP_HEADER_SIZE);
    last->prev = first->next;
    last->next = 0;
    last->size = 0;
    last->free = 0;

    // middle free block
    middle = first->next;
    middle->prev = first;
    middle->next = last;
    middle->size = chunk_size(middle);
    middle->free = 1;

    // store
    insert_free_chunk(middle);
    heap.first = first;
}
