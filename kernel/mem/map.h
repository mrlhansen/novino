#pragma once

#include <kernel/lists.h>

enum {
    MAP_FREE     = (1 << 0), // free region
    MAP_STACK    = (1 << 1), // memory grows downwards
    MAP_DIRECT   = (1 << 2), // one-to-one mapping of virtual and physical memory
    MAP_UC       = (1 << 3), // map as uncacheable
    MAP_WC       = (1 << 4), // map as write-combining
    MAP_EXEC     = (1 << 5), // map as executable
    MAP_READONLY = (1 << 6), // map as read-only
};

typedef struct {
    size_t flags;   // general flags
    size_t addr;    // virtual address of the mapping
    size_t phys;    // physical address, only present with direct maps
    size_t length;  // length of region
    link_t link;    // link in list of mappings
} mm_region_t;

int mm_map(list_t *map, size_t length, int flags, size_t *virt);
int mm_unmap(list_t *map, size_t virt);

int mm_remap_direct(list_t *map, size_t virt, size_t phys);
int mm_map_direct(list_t *map, size_t phys, size_t length, int flags, size_t *virt);

int mm_init(list_t *map, size_t addr, size_t length);
