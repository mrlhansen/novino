#pragma once

#include <kernel/types.h>

enum {
    E820_AVAILABLE    = 1,
    E820_RESERVED     = 2,
    E820_ACPI_RECLAIM = 3,
    E820_ACPI_NVS     = 4,
    E820_UNUSABLE     = 5,
    E820_DISABLED     = 6,
};

typedef struct {
    uint64_t start;
    uint64_t length;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed)) e820_region_t;

typedef struct {
    e820_region_t *region;
    uint32_t num_regions;
    uint64_t end_avail;
    uint64_t end_total;
    uint64_t mem_avail;
} e820_map_t;

void e820_report_available();
uint64_t e820_report_end(int avail);
void e820_init(size_t addr, size_t size);
