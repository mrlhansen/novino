#pragma once

#include <kernel/types.h>

enum {
    APIC_ID    = 0x020, // ID
    APIC_VER   = 0x030, // Version
    APIC_TPR   = 0x080, // Task Priority
    APIC_APR   = 0x090, // Arbitration Priority
    APIC_PPR   = 0x0A0, // Processor Priority
    APIC_EOI   = 0x0B0, // End of Interrupt
    APIC_RRD   = 0x0C0, // Remote Read
    APIC_LDR   = 0x0D0, // Logical Destination
    APIC_DFR   = 0x0E0, // Destination Format
    APIC_SIVR  = 0x0F0, // Spurious Interrupt Vector
    APIC_ISR   = 0x100, // In-Service (base)
    APIC_TMR   = 0x180, // Trigger Mode (base)
    APIC_IRR   = 0x200, // Interrupt Request (base)
    APIC_ESR   = 0x280, // Error Status
    APIC_LVT   = 0x2F0, // LVT CMCI (Corrected Machine Check Error Interrupt)
    APIC_ICR0  = 0x300, // Interrupt Command (low)
    APIC_ICR1  = 0x310, // Interrupt Command (high)
    APIC_LVTT  = 0x320, // LVT Timer
    APIC_LVTTS = 0x330, // Thermal Sensor
    APIC_LVTPC = 0x340, // Performance Monitoring Counters
    APIC_LVT0  = 0x350, // LVT LINT0 (Local Interrupt Pin 0)
    APIC_LVT1  = 0x360, // LVT LINT1 (Local Interrupt Pin 1)
    APIC_LVTER = 0x370, // LVT Error
    APIC_TICR  = 0x380, // Initial Count for Timer
    APIC_TCCR  = 0x390, // Current Count for Timer
    APIC_TDCR  = 0x3E0, // Divide Configuration for Timer
};

uint32_t lapic_read(uint16_t reg);
void lapic_write(uint16_t reg, uint32_t value);
void lapic_bcast_ipi(uint8_t vector, bool self, bool all);
void lapic_ipi(uint32_t apic_id, uint32_t type, uint8_t vector);
uint8_t lapic_get_id();

void lapic_timer_mask();
void lapic_timer_unmask();
void lapic_timer_init(uint32_t count, bool periodic);
uint64_t lapic_timer_calibrate();

void lapic_enable();
void lapic_init();
