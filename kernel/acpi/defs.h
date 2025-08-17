#pragma once

#include <kernel/types.h>

enum {
    GAS_SYSTEM_MEM    = 0,    // System Memory
    GAS_SYSTEM_IO     = 1,    // System I/O
    GAS_PCI_CFG_SPACE = 2,    // PCI Configuration Space
    GAS_EMBEDDED_CTRL = 3,    // Embedded Controller
    GAS_SMBUS         = 4,    // SMBus
    GAS_FUNC_FIXED_HW = 0x7F, // Functional Fixed Hardware
};

typedef struct {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t access_size;
    uint64_t address;
} __attribute__((packed)) gas_t;

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table[8];
    uint32_t oem_revision;
    char creator_id[4];
    uint32_t creator_revision;
} __attribute__((packed)) header_t;

typedef struct {
    header_t hdr;
    uint32_t table_ptr[0];
} __attribute__((packed)) rsdt_t;

typedef struct {
    header_t hdr;
    uint64_t table_ptr[0];
} __attribute__((packed)) xsdt_t;

typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t xchecksum;
    uint8_t reserved[3];
} __attribute__((packed)) rsdp_t;
