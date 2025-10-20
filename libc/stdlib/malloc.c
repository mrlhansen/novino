#include <novino/syscalls.h>
#include <stdlib.h>
#include <string.h>

#define HEAP_SBRK_ALIGN 4096
#define HEAP_CHUNK_SIZE sizeof(chunk_t)
#define HEAP_DATA_ALIGN (2*sizeof(size_t))

#define align_size(s,a) ((s + a - 1) & -(a)) // 2s complement
#define chunk_size(c) ((size_t)c->next - (size_t)c - HEAP_CHUNK_SIZE)
#define chunk_next(c,s) ((void*)c + HEAP_CHUNK_SIZE + s)

typedef struct chunk {
    struct chunk *prev;
    struct chunk *next;
    size_t size;
    size_t free;
} chunk_t;

static chunk_t *heap_free_list = NULL;
static size_t heap_start = 0;
static size_t heap_end = 0;

static int heap_sbrk(long sbrk)
{
    long status;

    if(sbrk < 0)
    {
        return -1;
    }

    sbrk = align_size(sbrk, HEAP_SBRK_ALIGN);
    status = sys_brk(heap_end + sbrk);
    if(status < 0)
    {
        return -1;
    }
    heap_end = status;

    return 0;
}

static void insert_free_chunk(chunk_t *chunk)
{
    chunk->free = 1;

    chunk[1].next = heap_free_list;
    chunk[1].prev = 0;

    if(heap_free_list)
    {
        heap_free_list[1].prev = chunk;
    }

    heap_free_list = chunk;
}

static void remove_free_chunk(chunk_t *chunk)
{
    chunk->free = 0;

    if(heap_free_list == chunk)
    {
        heap_free_list = chunk[1].next;
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
    if(chunk->size < size + HEAP_DATA_ALIGN + HEAP_CHUNK_SIZE)
    {
        return;
    }

    // new free chunk
    next = chunk_next(chunk, size);
    next->prev = chunk;
    next->next = chunk->next;
    next->size = chunk_size(next);

    // adjust chunk
    chunk->next->prev = next;
    chunk->next = next;
    chunk->size = size;

    // insert
    insert_free_chunk(next);
}

static void glue_chunk(chunk_t *chunk, int prev, int next)
{
    int free;

    // backwards
    if(prev && chunk->prev->free)
    {
        remove_free_chunk(chunk);
        remove_free_chunk(chunk->prev);

        chunk->prev->size += chunk->size + HEAP_CHUNK_SIZE;
        chunk->prev->next = chunk->next;
        chunk->next->prev = chunk->prev;

        insert_free_chunk(chunk->prev);
        chunk = chunk->prev;
    }

    // forwards
    if(next && chunk->next->free)
    {
        free = chunk->free;
        if(free)
        {
            remove_free_chunk(chunk);
        }
        remove_free_chunk(chunk->next);

        chunk->size += chunk->next->size + HEAP_CHUNK_SIZE;
        chunk->next = chunk->next->next;
        chunk->next->prev = chunk;

        if(free)
        {
            insert_free_chunk(chunk);
        }
    }
}

static chunk_t *heap_expand(long size)
{
    chunk_t *old, *new;
    int status;

    // make room for header
    size = size + HEAP_CHUNK_SIZE;

    // allocate memory
    old = (void*)(heap_end - HEAP_CHUNK_SIZE);
    status = heap_sbrk(size);
    if(status < 0)
    {
        return NULL;
    }
    new = (void*)(heap_end - HEAP_CHUNK_SIZE);

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
    chunk = heap_free_list;
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

int __libc_heap_init()
{
    chunk_t *first, *last, *middle;
    long status;

    status = sys_brk(0);
    if(status < 0)
    {
        return 0;
    }
    heap_start = status;
    heap_end = status;

    status = heap_sbrk(HEAP_SBRK_ALIGN);
    if(status < 0)
    {
        return status;
    }

    // front guard block
    first = (void*)heap_start;
    first->prev = 0;
    first->next = first + 1;
    first->size = 0;
    first->free = 0;

    // rear guard block
    last = (void*)(heap_end - HEAP_CHUNK_SIZE);
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

    insert_free_chunk(middle);
    return 0;
}

void *malloc(size_t size)
{
    chunk_t *chunk;

    // always returns NULL
    if(size == 0)
    {
        return NULL;
    }

    // round size to alignment
    size = align_size(size, HEAP_DATA_ALIGN);

    // find a free chunk
    chunk = find_free_chunk(size);
    if(!chunk)
    {
        return NULL;
    }

    // split chunk if necessary
    split_chunk(chunk, size);

    // remove chunk from free list
    remove_free_chunk(chunk);

    // pointer to data region
    return (chunk + 1);
}

void free(void *ptr)
{
    chunk_t *chunk;

    // invalid
    if(ptr < (void*)heap_start)
    {
        return;
    }

    if(ptr > (void*)heap_end)
    {
        return;
    }

    // pointer to chunk
    chunk = ptr - HEAP_CHUNK_SIZE;

    // insert as free
    insert_free_chunk(chunk);

    // glue chunks when possible
    glue_chunk(chunk, 1, 1);
}

void *calloc(size_t num, size_t size)
{
    void *ptr;

    // always returns NULL
    if((num == 0) || (size == 0))
    {
        return NULL;
    }

    // allocate and clear memory
    ptr = malloc(num * size);
    if(ptr)
    {
        memset(ptr, 0, num * size);
    }

    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    size_t orig_size;
    chunk_t *chunk;
    void *new_ptr;

    // special cases
    if(ptr == NULL)
    {
        return malloc(size);
    }

    if(size == 0)
    {
        free(ptr);
        return NULL;
    }

    // variables
    size = align_size(size, HEAP_DATA_ALIGN);
    chunk = ptr - HEAP_CHUNK_SIZE;
    orig_size = chunk->size;

    // shrinking
    if(size <= chunk->size)
    {
        split_chunk(chunk, size);
        return ptr;
    }

    // merge with next block if free
    glue_chunk(chunk, 0, 1);
    if(size <= chunk->size)
    {
        split_chunk(chunk, size);
        return ptr;
    }

    // this is the last chunk, so we can allocate more memory
    if(chunk->next->size == 0)
    {
        if(heap_expand(size) == NULL)
        {
            return NULL;
        }
        glue_chunk(chunk, 0, 1);
        split_chunk(chunk, size);
        return ptr;
    }

    // malloc and copy
    new_ptr = malloc(size);
    if(new_ptr)
    {
        memcpy(new_ptr, ptr, orig_size);
        free(ptr);
    }
    return new_ptr;
}
