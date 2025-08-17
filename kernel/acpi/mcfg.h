#pragma once

#include <kernel/acpi/defs.h>

typedef struct {
    uint64_t base_address;
    uint16_t segment_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} __attribute__((packed)) mcfg_entry_t;

typedef struct {
    header_t hdr;
    uint64_t reserved;
    mcfg_entry_t entry[0];
} __attribute__((packed)) mcfg_t;

void acpi_mcfg_parse(uint64_t);
