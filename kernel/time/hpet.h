#pragma once

#include <kernel/types.h>

enum {
    HPET_CAP      = 0x000,  // Capabilities and ID Register
    HPET_CNFG     = 0x010,  // Configuration Register
    HPET_IS       = 0x020,  // Interrupt Status Register
    HPET_CNT      = 0x0F0,  // Main Counter Value Register
    HPET_TM0_CCR  = 0x100,  // Timer 0 Configuration and Capability Register
    HPET_TM0_CVR  = 0x108,  // Timer 0 Comparator Value Register
    HPET_TM0_FIRR = 0x110   // Timer 0 FSB Interrupt Route Register
};

typedef struct {
    uint8_t rev_id;               // Revision ID, must be non-zero
    uint8_t num_tim_cap    : 5;   // Number of timers minus one (i.e. a value of 2 means 3 timers in total)
    uint8_t count_size_cap : 1;   // Counter size, 0 = 32-bit, 1 = 64-bit
    uint8_t reserved       : 1;   // Reserved
    uint8_t leg_route_cap  : 1;   // LegacyReplacement Route Capable
    uint16_t vendor_id;           // PCI vendor ID (read-only)
    uint32_t counter_clk_period;  // Main Counter Tick Period (in femtoseconds)
} __attribute__((packed)) hpet_cap_t;

typedef struct {
    uint8_t  rsv0            : 1;  // Reserved
    uint8_t  int_type_cnf    : 1;  // 0 = Edge triggered, 1 = Level triggered
    uint8_t  int_enb_cnf     : 1;  // Enable interrupts
    uint8_t  type_cnf        : 1;  // 0 = One-shot mode, 1 = Periodic mode
    uint8_t  per_int_cap     : 1;  // Periodic mode supported
    uint8_t  size_cap        : 1;  // 0 = 32-bit, 1 = 64-bit
    uint8_t  val_set_cnf     : 1;  // Allow setting accumulator directly when using periodic mode
    uint8_t  rsv1            : 1;  // Reserved
    uint8_t  mode32_cnf      : 1;  // Force 32-bit mode
    uint8_t  int_route_cnf   : 5;  // I/O APIC pin used for interrupt
    uint8_t  fsb_en_cnf      : 1;  // Enable FSB interrupts
    uint8_t  fsb_int_del_cap : 1;  // FSB interrupts supported
    uint16_t rsv3;                 // Reserved
    uint32_t int_route_cap;        // Possible I/O APIC pins
} __attribute__((packed)) hpet_ccr_t;

typedef struct {
    uint8_t id;
    uint8_t free;
    uint32_t route;
    uint8_t fsb;
    uint8_t size;
    uint8_t periodic;
} hpet_timer_t;

uint64_t hpet_timestamp();
int hpet_init();
