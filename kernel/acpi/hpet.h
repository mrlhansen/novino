#pragma once

#include <kernel/acpi/defs.h>

typedef struct {
    header_t hdr;
    uint8_t hw_revision_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved1 : 1;
    uint8_t legacy_replacement : 1;
    uint16_t pci_vendor_id;
    gas_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed)) hpet_t;

void acpi_hpet_parse(uint64_t);
