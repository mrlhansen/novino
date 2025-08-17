#pragma once

#include <kernel/acpi/defs.h>

typedef struct {
    header_t hdr;
    uint8_t reserved[12];
} __attribute__((packed)) srat_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t lo_pdm;
    uint8_t apic_id;
    uint32_t flags;
    uint8_t sapic_eid;
    uint32_t hi_pdm : 24;
    uint32_t cdm;
} __attribute__((packed)) srat_apic_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint32_t pdm;
    uint16_t reserved1;
    uint64_t range_base;
    uint64_t range_length;
    uint32_t reserved2;
    uint32_t flags;
    uint64_t reserved3;
} __attribute__((packed)) srat_mem_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t reserved1;
    uint32_t pdm;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t cdm;
    uint32_t reserved2;
} __attribute__((packed)) srat_x2apic_t;

void acpi_srat_parse(uint64_t);
