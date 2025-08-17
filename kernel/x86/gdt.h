#pragma once

#include <kernel/types.h>

typedef struct {
    uint64_t value;
} __attribute__((packed)) gdt_entry_t;

typedef struct  {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdtr_t;

void gdt_set_gate(uint8_t, uint64_t);
void gdt_init();
void gdt_load();
