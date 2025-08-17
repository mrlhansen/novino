#pragma once

#include <kernel/acpi/defs.h>

enum {
    MADT_LAPIC       = 0,  // Processor Local APIC
    MADT_IOAPIC      = 1,  // IO APIC
    MADT_ISO         = 2,  // Interrupt Source Override
    MADT_NMI         = 3,  // Non-maskable Interrupt Source
    MADT_LAPIC_NMI   = 4,  // Local APIC NMI
    MADT_LAPIC_AO    = 5,  // Local APIC Address Override
    MADT_IOSAPIC     = 6,  // IO SAPIC
    MADT_LSAPIC      = 7,  // Local SAPIC
    MADT_PIS         = 8,  // Platform Interrupt Sources
    MADT_LX2APIC     = 9,  // Local x2APIC
    MADT_LX2APIC_NMI = 10, // Local x2APIC NMI
    MADT_GIC         = 11, // Generic Interrupt Controller (ARM platform)
    MADT_GICD        = 12  // GIC Distributor (ARM platform)
};

typedef struct {
    header_t hdr;
    uint32_t lapic_address;
    uint32_t flags;
} __attribute__((packed)) madt_t;

typedef struct{
    uint8_t type;
    uint8_t length;
    uint8_t acpi_id;
    uint8_t lapic_id;
    uint32_t flags;
} __attribute__((packed)) madt_lapic_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_address;
    uint32_t gsi_base;
} __attribute__((packed)) madt_ioapic_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t bus;
    uint8_t source;
    uint32_t gsi;
    uint8_t polarity  : 2;
    uint8_t triggered : 2;
    uint16_t reserved : 12;
} __attribute__((packed)) madt_iso_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t flags;
    uint32_t gsi;
} __attribute__((packed)) madt_nmi_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t acpi_id;
    uint16_t flags;
    uint8_t lapic_lint;
} __attribute__((packed)) madt_lapic_nmi_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t reserved;
    uint64_t lapic_address;
} __attribute__((packed)) madt_lapic_ao_t;

void acpi_madt_parse(uint64_t);
