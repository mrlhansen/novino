#pragma once

#include <kernel/types.h>

#define IOAPIC_ID     0x00 // ID
#define IOAPIC_VER    0x01 // Version
#define IOAPIC_ARB    0x02 // Arbitration ID
#define IOAPIC_REDTBL 0x10 // Redirection Entry Table Offset

// Trigger mode
enum {
    EDGE  = 0,
    LEVEL = 1
};

// Destination mode
enum {
    PHYSICAL = 0,
    LOGICAL  = 1
};

// Pin polarity
enum {
    HIGH = 0,
    LOW  = 1
};

// Delivery modes
enum {
    FIXED  = 0,
    LOWEST = 1,
    SMI    = 2,
    NMI    = 4,
    INIT   = 5,
    SIPI   = 6,
};

// Redirection entry
typedef struct {
    uint32_t vector        : 8;   // Vector
    uint32_t delv_mode     : 3;   // Delivery mode
    uint32_t dest_mode     : 1;   // Destination mode (0 = physical, 1 = logical)
    uint32_t delv_status   : 1;   // Delivery status
    uint32_t polarity      : 1;   // Polarity (0 = high, 1 = low)
    uint32_t remote_irr    : 1;   // Remote IRR
    uint32_t mode          : 1;   // Trigger mode (0 = edge, 1 = level)
    uint32_t mask          : 1;   // Mask
    uint64_t reserved      : 39;  // Reserved
    uint32_t destination   : 8;   // Destination
} __attribute__((packed)) redir_t;

typedef struct {
    uint8_t  apic_id;
    uint64_t address;
    uint16_t gsi_base;
    uint16_t max_entries;
    uint16_t version;
} ioapic_t;

extern int ioapic_max_gsi;

void ioapic_enable(uint8_t, uint32_t, uint64_t);
void ioapic_route(uint32_t, redir_t*);
void ioapic_init();
