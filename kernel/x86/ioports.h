#pragma once

#include <kernel/types.h>

void outportb(uint16_t, uint8_t);
uint8_t inportb(uint16_t);

void outportw(uint16_t, uint16_t);
uint16_t inportw(uint16_t);

void outportl(uint16_t, uint32_t);
uint32_t inportl(uint16_t);

uint64_t read_msr(uint32_t);
void write_msr(uint32_t, uint64_t);
