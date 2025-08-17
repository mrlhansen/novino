#pragma once

#include <kernel/x86/tss.h>
#include <kernel/types.h>

typedef struct {
    uint8_t present;
    uint8_t bsp;
    uint8_t apic_id;
    uint16_t tr;
    tss_t tss;
} core_t;

extern uint64_t ap_boot_code;
extern uint16_t ap_boot_size;

int smp_core_apic_id(int id);
int smp_core_id();
int smp_core_count();
void smp_enable_core(int id);
void smp_init();
