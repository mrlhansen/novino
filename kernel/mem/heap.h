#pragma once

#include <kernel/types.h>

typedef struct chunk {
    struct chunk *prev;
    struct chunk *next;
    size_t size;
    size_t free;
} chunk_t;

typedef struct {
    chunk_t *first; // First chunk
    chunk_t *free;  // First free chunk
    size_t start;   // Start addresss of the heap
    size_t size;    // Total size of the heap
    size_t end;     // End address of the heap
} heap_t;

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void kfree(void *ptr);

size_t heap_get_size();
void heap_init();
